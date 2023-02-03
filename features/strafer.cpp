// Vectorial Strafer

#include <ICvar.h>
#include <IUtils.h>
#include <IHooks.h>
#include <convar.h>
#include <dbg.h>

#include <hl_sdk/engine/APIProxy.h>

#include "strafer.h"

#include "../game/utils.h"

#include "../config.h"

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

CStrafer g_Strafer;

Strafe::StrafeData g_strafeData;

cvar_s *sv_friction = NULL;
cvar_s *sv_accelerate = NULL;
cvar_s *sv_airaccelerate = NULL;
cvar_s *sv_stopspeed = NULL;

//-----------------------------------------------------------------------------
// Updated strafe data
//-----------------------------------------------------------------------------

void UpdateStrafeData(Strafe::StrafeData &strafeData, bool bStrafe, Strafe::StrafeDir dir, Strafe::StrafeType type, float flYaw, float flPointX, float flPointY)
{
	*reinterpret_cast<Vector *>(strafeData.player.Velocity) = g_pPlayerMove->velocity;
	*reinterpret_cast<Vector *>(strafeData.player.Origin) = g_pPlayerMove->origin;

	strafeData.vars.OnGround = g_pPlayerMove->onground != -1;
	strafeData.vars.EntFriction = g_pPlayerMove->friction;
	strafeData.vars.Maxspeed = g_pPlayerMove->maxspeed;
	strafeData.vars.ReduceWishspeed = strafeData.vars.OnGround && (g_pPlayerMove->flags & FL_DUCKING);

	if (g_pPlayerMove->movevars)
	{
		strafeData.vars.Maxspeed = g_pPlayerMove->movevars->maxspeed;
		strafeData.vars.Stopspeed = g_pPlayerMove->movevars->stopspeed;
		strafeData.vars.Friction = g_pPlayerMove->movevars->friction;
		strafeData.vars.Accelerate = g_pPlayerMove->movevars->accelerate;
		strafeData.vars.Airaccelerate = g_pPlayerMove->movevars->airaccelerate;
	}
	else
	{
		strafeData.vars.Stopspeed = sv_stopspeed->value;
		strafeData.vars.Friction = sv_friction->value;
		strafeData.vars.Accelerate = sv_accelerate->value;
		strafeData.vars.Airaccelerate = sv_airaccelerate->value;
	}

	strafeData.vars.Frametime = g_pPlayerMove->frametime; // 1.0f / 200.0f (1.0f / fps_max)

	strafeData.frame.Strafe = bStrafe;
	strafeData.frame.SetDir( dir );
	strafeData.frame.SetType( type );

	strafeData.frame.SetX( flPointX );
	strafeData.frame.SetY( flPointY );

	//if (!*strafe_yaw->string)
		strafeData.frame.SetYaw(static_cast<double>(flYaw));
	//else
	//	strafeData.frame.SetYaw(static_cast<double>(strafe_yaw->value));
}

//-----------------------------------------------------------------------------
// ConCommands/CVars
//-----------------------------------------------------------------------------

ConVar sc_strafe_point_x("sc_strafe_point_x", "0", FCVAR_CLIENTDLL, "Coordinate X to point strafe");
ConVar sc_strafe_point_y("sc_strafe_point_y", "0", FCVAR_CLIENTDLL, "Coordinate Y to point strafe");

ConVar sc_strafe_ignore_ground("sc_strafe_ignore_ground", "1", FCVAR_CLIENTDLL, "Don't strafe when on ground");
ConVar sc_strafe_dir("sc_strafe_dir", "3", FCVAR_CLIENTDLL, "Strafing direction. Directions:\n\t0 - to the left\n\t1 - to the right\n\t2 - best strafe\n\t3 - to view angles\n\t4 - to the point", true, 0.f, true, 4.f);
ConVar sc_strafe_type("sc_strafe_type", "0", FCVAR_CLIENTDLL, "Strafing type. Types:\n\t0 - Max acceleration strafing\n\t1 - Max angle strafing\n\t2 - Max deceleration strafing\n\t3 - Const speed strafing", true, 0.f, true, 3.f);

static void CvarChangeHook_sc_strafe_ignore_ground(cvar_t *pCvar, const char *pszOldValue, float flOldValue)
{
	Utils()->PrintChatText("<SvenInt> Strafe on ground is %s\n", !(pCvar->value) ? "ON" : "OFF");
}

static void CvarChangeHook_sc_strafe_dir(cvar_t *pCvar, const char *pszOldValue, float flOldValue)
{
	//if ( pCvar->value == flOldValue )
	//	return;

	if ( sc_strafe_dir.Clamp() )
		return;

	const char *pszDir = NULL;

	switch ( (int)pCvar->value )
	{
	case 0:
		pszDir = "To the left";
		break;
		
	case 1:
		pszDir = "To the right";
		break;
		
	case 2:
		pszDir = "Best strafe";
		break;
		
	case 3:
		pszDir = "To view angles";
		break;
		
	case 4:
		pszDir = "To the point";
		break;

	default:
		return;
	}

	Utils()->PrintChatText("<SvenInt> Strafe direction is %s\n", pszDir);
}

static void CvarChangeHook_sc_strafe_type(cvar_t *pCvar, const char *pszOldValue, float flOldValue)
{
	//if ( pCvar->value == flOldValue )
	//	return;

	if ( sc_strafe_type.Clamp() )
		return;

	const char *pszType = NULL;

	switch ( (int)pCvar->value )
	{
	case 0:
		pszType = "Max acceleration";
		break;
		
	case 1:
		pszType = "Max angle";
		break;
		
	case 2:
		pszType = "Max deceleration";
		break;
		
	case 3:
		pszType = "Const speed";
		break;

	default:
		return;
	}

	Utils()->PrintChatText("<SvenInt> Strafe type is %s\n", pszType);
}

CON_COMMAND_NO_WRAPPER(sc_strafe, "Toggle Vectorial Strafing")
{
	Msg(g_Config.cvars.strafe ? "Vectorial Strafer disabled\n" : "Vectorial Strafer enabled\n");
	g_Config.cvars.strafe = !g_Config.cvars.strafe;

	Utils()->PrintChatText("<SvenInt> Strafer is %s\n", g_Config.cvars.strafe ? "ON" : "OFF");
}

//-----------------------------------------------------------------------------
// CStrafer implementations
//-----------------------------------------------------------------------------

void CStrafer::OnConfigLoad()
{
	sc_strafe_ignore_ground.SetValue( g_Config.cvars.strafe_ignore_ground );
	sc_strafe_dir.SetValue( g_Config.cvars.strafe_dir );
	sc_strafe_type.SetValue( g_Config.cvars.strafe_type );
}

void CStrafer::OnConfigSave()
{
	g_Config.cvars.strafe_ignore_ground = sc_strafe_ignore_ground.GetBool();
	g_Config.cvars.strafe_dir = sc_strafe_dir.GetInt();
	g_Config.cvars.strafe_type = sc_strafe_type.GetInt();
}

void CStrafer::CreateMove(float frametime, struct usercmd_s *cmd, int active)
{
	StrafeVectorial(cmd);
}

void CStrafer::StrafeVectorial(struct usercmd_s *cmd)
{
	if ( sc_strafe_ignore_ground.GetBool() && g_pPlayerMove->onground != -1 || cmd->buttons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT) )
		return;

	if ( g_Config.cvars.antiafk || g_pPlayerMove->dead || g_pPlayerMove->iuser1 != 0 || g_pPlayerMove->movetype != MOVETYPE_WALK || g_pPlayerMove->waterlevel > WL_FEET )
		return;

	Vector va;
	g_pEngineFuncs->GetViewAngles(va);

	UpdateStrafeData(g_strafeData,
					 g_Config.cvars.strafe,
					 static_cast<Strafe::StrafeDir>( sc_strafe_dir.GetInt() ),
					 static_cast<Strafe::StrafeType>( sc_strafe_type.GetInt() ),
					 va[1],
					 sc_strafe_point_x.GetFloat(),
					 sc_strafe_point_y.GetFloat());

	if (g_strafeData.frame.Strafe)
	{
		Strafe::ProcessedFrame out;
		out.Yaw = va[1];

		Strafe::Friction(g_strafeData);

		Strafe::StrafeVectorial(g_strafeData, out, false);

		if (out.Processed)
		{
			cmd->forwardmove = out.Forwardspeed;
			cmd->sidemove = out.Sidespeed;

			va[1] = static_cast<float>(out.Yaw);
		}
	}

	g_pEngineFuncs->SetViewAngles(va);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool CStrafer::Load()
{
	sv_friction = CVar()->FindCvar("sv_friction");
	sv_accelerate = CVar()->FindCvar("sv_accelerate");
	sv_airaccelerate = CVar()->FindCvar("sv_airaccelerate");
	sv_stopspeed = CVar()->FindCvar("sv_stopspeed");

	if ( !sv_friction )
	{
		Warning("Can't find cvar \"sv_friction\"\n");
		return false;
	}
	
	if ( !sv_accelerate )
	{
		Warning("Can't find cvar \"sv_accelerate\"\n");
		return false;
	}
	
	if ( !sv_airaccelerate )
	{
		Warning("Can't find cvar \"sv_airaccelerate\"\n");
		return false;
	}
	
	if ( !sv_stopspeed )
	{
		Warning("Can't find cvar \"sv_stopspeed\"\n");
		return false;
	}

	g_strafeData.frame.UseGivenButtons = true;
	g_strafeData.frame.buttons = Strafe::StrafeButtons();
	g_strafeData.frame.buttons.AirLeft = Strafe::Button::LEFT;
	g_strafeData.frame.buttons.AirRight = Strafe::Button::RIGHT;

	return true;
}

void CStrafer::PostLoad()
{
	Hooks()->HookCvarChange( CVar()->FindCvar("sc_strafe_ignore_ground"), CvarChangeHook_sc_strafe_ignore_ground );
	Hooks()->HookCvarChange( CVar()->FindCvar("sc_strafe_dir"), CvarChangeHook_sc_strafe_dir );
	Hooks()->HookCvarChange( CVar()->FindCvar("sc_strafe_type"), CvarChangeHook_sc_strafe_type );
}

void CStrafer::Unload()
{
	Hooks()->UnhookCvarChange( CVar()->FindCvar("sc_strafe_ignore_ground"), CvarChangeHook_sc_strafe_ignore_ground );
	Hooks()->UnhookCvarChange( CVar()->FindCvar("sc_strafe_dir"), CvarChangeHook_sc_strafe_dir );
	Hooks()->UnhookCvarChange( CVar()->FindCvar("sc_strafe_type"), CvarChangeHook_sc_strafe_type );
}