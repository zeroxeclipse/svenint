// Thirdperson

#include <hl_sdk/engine/APIProxy.h>

#include <ISvenModAPI.h>
#include <client_state.h>
#include <convar.h>
#include <dbg.h>
#include <keydefs.h>

#include "thirdperson.h"

#include "camhack.h"

#include "../game/utils.h"
#include "../config.h"

extern Vector g_oldviewangles;
extern Vector g_newviewangles;
extern Vector g_vecSpinAngles;

extern cvar_t *hud_draw;

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

CThirdPerson g_ThirdPerson;

static usercmd_t dummy_cmd;

static bool keydown_w = false;
static bool keydown_s = false;
static bool keydown_a = false;
static bool keydown_d = false;
static bool keydown_space = false;
static bool keydown_ctrl = false;
static bool keydown_shift = false;
static bool keydown_mouse1 = false;
static bool keydown_mouse2 = false;

//-----------------------------------------------------------------------------
// ConVars, ConCommands
//-----------------------------------------------------------------------------

ConVar sc_thirdperson("sc_thirdperson", "0", FCVAR_CLIENTDLL, "Use enhanced third person");
ConVar sc_thirdperson_edit_mode("sc_thirdperson_edit_mode", "0", FCVAR_CLIENTDLL, "Allow change position and angles of third person camera");
ConVar sc_thirdperson_hidehud("sc_thirdperson_hidehud", "0", FCVAR_CLIENTDLL, "Hide HUD when third person enabled");
ConVar sc_thirdperson_ignore_pitch("sc_thirdperson_ignore_pitch", "0", FCVAR_CLIENTDLL, "Ignore the Pitch angle");
ConVar sc_thirdperson_ignore_yaw("sc_thirdperson_ignore_yaw", "0", FCVAR_CLIENTDLL, "Ignore the Yaw angle");

CON_COMMAND_EXTERN_NO_WRAPPER(sc_thirdperson_reset_position, ConCommand_ThirdPerson_ResetPosition, "Reset position of third person camera")
{
	g_Config.cvars.thirdperson_origin[0] = -64.f;
	g_Config.cvars.thirdperson_origin[1] = 0.f;
	g_Config.cvars.thirdperson_origin[2] = 12.f;
}

CON_COMMAND_EXTERN_NO_WRAPPER(sc_thirdperson_reset_angles, ConCommand_ThirdPerson_ResetAngles, "Reset angles of third person camera")
{
	g_Config.cvars.thirdperson_angles[0] = 0.f;
	g_Config.cvars.thirdperson_angles[1] = 0.f;
	g_Config.cvars.thirdperson_angles[2] = 0.f;
}

//-----------------------------------------------------------------------------
// Third Person
//-----------------------------------------------------------------------------

void CThirdPerson::OnConfigLoad()
{
	sc_thirdperson.SetValue( g_Config.cvars.thirdperson );
	sc_thirdperson_edit_mode.SetValue( g_Config.cvars.thirdperson_edit_mode );
	sc_thirdperson_hidehud.SetValue( g_Config.cvars.thirdperson_hidehud );
	sc_thirdperson_ignore_pitch.SetValue( g_Config.cvars.thirdperson_ignore_pitch );
	sc_thirdperson_ignore_yaw.SetValue( g_Config.cvars.thirdperson_ignore_yaw );

	m_bEditMode = g_Config.cvars.thirdperson_edit_mode;
	m_bThirdPerson = ( g_Config.cvars.thirdperson && (SvenModAPI()->GetClientState() == CLS_ACTIVE && g_pClientFuncs->CL_IsThirdPerson()) );

	if (m_bThirdPerson && g_Config.cvars.thirdperson_hidehud)
	{
		CVar()->SetValue( hud_draw, false );
	}
}

void CThirdPerson::CreateMove(float frametime, struct usercmd_s *cmd, int active)
{
	bool bInThirdPerson = ( g_Config.cvars.thirdperson && g_pClientFuncs->CL_IsThirdPerson() );

	if ( m_bThirdPerson != bInThirdPerson )
	{
		if ( bInThirdPerson )
		{
			if ( !g_CamHack.IsEnabled() && g_Config.cvars.thirdperson_hidehud )
			{
				CVar()->SetValue( hud_draw, false );
			}
		}
		else
		{
			if ( !g_CamHack.IsEnabled() )
			{
				CVar()->SetValue( hud_draw, true );
			}

			m_vecViewAngles = g_newviewangles;
		}

		m_bThirdPerson = bInThirdPerson;
	}

	if ( m_bEditMode != g_Config.cvars.thirdperson_edit_mode )
	{
		if ( g_Config.cvars.thirdperson_edit_mode )
		{
			keydown_w = false;
			keydown_s = false;
			keydown_a = false;
			keydown_d = false;
			keydown_space = false;
			keydown_ctrl = false;
			keydown_shift = false;
			keydown_mouse1 = false;
			keydown_mouse2 = false;

			bool bAnglesChanged = false;

			if (g_Config.cvars.spin_yaw_angle)
			{
				bAnglesChanged = true;
			}
			else if (g_Config.cvars.lock_yaw)
			{
				bAnglesChanged = true;
			}

			if (g_Config.cvars.spin_pitch_angle)
			{
				bAnglesChanged = true;
			}
			else if (g_Config.cvars.lock_pitch)
			{
				bAnglesChanged = true;
			}

			if (bAnglesChanged)
			{
				m_vecViewAngles = g_vecSpinAngles;
			}
			else
			{
				g_pEngineFuncs->GetViewAngles(m_vecViewAngles);
			}

			m_flSavedPitchAngle = NormalizeAngle(m_vecViewAngles.x) / -3.0f;
		}
		else
		{
			g_pEngineFuncs->SetViewAngles(m_vecViewAngles);
		}

		m_bEditMode = g_Config.cvars.thirdperson_edit_mode;
	}

	if ( g_Config.cvars.thirdperson && g_Config.cvars.thirdperson_edit_mode && !g_CamHack.IsEnabled() )
	{
		float flMaxSpeed = Client()->GetMaxSpeed();

		bool bAnglesChanged = false;

		if (g_Config.cvars.spin_yaw_angle)
		{
			bAnglesChanged = true;
		}
		else if (g_Config.cvars.lock_yaw)
		{
			bAnglesChanged = true;
		}

		if (g_Config.cvars.spin_pitch_angle)
		{
			bAnglesChanged = true;
		}
		else if (g_Config.cvars.lock_pitch)
		{
			bAnglesChanged = true;
		}

		dummy_cmd.forwardmove = 0.f;
		dummy_cmd.sidemove = 0.f;
		dummy_cmd.upmove = 0.f;

		if (keydown_shift)
			flMaxSpeed /= 2;

		if (keydown_w)
			dummy_cmd.forwardmove += flMaxSpeed;
		
		if (keydown_s)
			dummy_cmd.forwardmove -= flMaxSpeed;
		
		if (keydown_d)
			dummy_cmd.sidemove += flMaxSpeed;
		
		if (keydown_a)
			dummy_cmd.sidemove -= flMaxSpeed;

		if (keydown_space)
			dummy_cmd.upmove += flMaxSpeed;
		
		if (keydown_ctrl)
			dummy_cmd.upmove -= flMaxSpeed;

		if (keydown_mouse1)
			g_Config.cvars.thirdperson_angles[2] -= 0.2f;

		if (keydown_mouse2)
			g_Config.cvars.thirdperson_angles[2] += 0.2f;

		dummy_cmd.upmove *= 0.75f;

		Vector va_delta = g_newviewangles - g_oldviewangles;

		// ToDo: for better rotation, use quaternions when the camera is tilted
		*reinterpret_cast<Vector *>(g_Config.cvars.thirdperson_angles) += va_delta;

		NormalizeAngles( g_Config.cvars.thirdperson_angles );
		ClampViewAngles();

		PM_NoClip( &dummy_cmd );

		cmd->viewangles = bAnglesChanged ? g_vecSpinAngles : m_vecViewAngles;

		cmd->forwardmove = 0.f;
		cmd->sidemove = 0.f;

		cmd->buttons &= ~(0xFFFFFFFF & ~IN_DUCK);
	}
	else
	{
		NormalizeAngles( g_Config.cvars.thirdperson_angles );
		ClampViewAngles();
	}
}

void CThirdPerson::V_CalcRefdef(struct ref_params_s *pparams)
{
	if ( g_Config.cvars.thirdperson && !g_CamHack.IsEnabled() && g_pClientFuncs->CL_IsThirdPerson() )
	{
		if ( SvenModAPI()->GetClientState() == CLS_ACTIVE && !Client()->IsSpectating() )
		{
			float localToWorld[3][4];
			Vector vecOrigin, vecAngles, vecForward, va, tmp;
			Vector vecEyePosition = Client()->GetOrigin() + Client()->GetViewOffset();
			cl_entity_t *pLocal = g_pEngineFuncs->GetLocalPlayer();

			if ( g_Config.cvars.thirdperson_edit_mode )
				va = m_vecViewAngles;
			else
				g_pEngineFuncs->GetViewAngles(va);

			if ( g_Config.cvars.thirdperson_ignore_pitch )
				va.x = 0.f;
			
			if ( g_Config.cvars.thirdperson_ignore_yaw )
				va.y = 0.f;

			va.z = 0.f;

			AngleVectors( g_Config.cvars.thirdperson_angles, &vecForward, NULL, NULL );
			AngleMatrix( va, localToWorld );

			VectorTransform( vecForward, localToWorld, tmp );

			localToWorld[0][3] = vecEyePosition.x;
			localToWorld[1][3] = vecEyePosition.y;
			localToWorld[2][3] = vecEyePosition.z;

			VectorTransform( g_Config.cvars.thirdperson_origin, localToWorld, vecOrigin );

			vecAngles.x = -atan2f(tmp.z, tmp.Length2D()) * (float)(180.0 / M_PI);
			vecAngles.y = atan2f(tmp.y, tmp.x) * (float)(180.0 / M_PI);
			vecAngles.z = g_Config.cvars.thirdperson_angles[2];

			if ( g_Config.cvars.thirdperson_clip_to_wall )
			{
				pmtrace_t trace;
				Vector vecOffset(0.f, 0.f, (Client()->IsDucking() ? VEC_DUCK_HULL_MAX.z : VEC_HULL_MAX.z) / 2.f);

				g_pEventAPI->EV_SetTraceHull( g_Config.cvars.thirdperson_trace_type == 0 ? PM_HULL_POINT : PM_HULL_PLAYER );
				g_pEventAPI->EV_PlayerTrace( Client()->GetOrigin() + vecOffset, vecOrigin, PM_STUDIO_IGNORE, -1, &trace );

				vecOrigin = trace.endpos;
			}

			*reinterpret_cast<Vector *>(pparams->vieworg) = vecOrigin;
			*reinterpret_cast<Vector *>(pparams->viewangles) = vecAngles;

			if ( g_Config.cvars.thirdperson_edit_mode )
			{
				pLocal->angles.x = m_flSavedPitchAngle;
				pLocal->curstate.angles.x = m_flSavedPitchAngle;
				pLocal->prevstate.angles.x = m_flSavedPitchAngle;
				pLocal->latched.prevangles.x = m_flSavedPitchAngle;
			}
		}
	}
}

bool CThirdPerson::OnKeyPress(int down, int keynum)
{
	bool bKeyDown = (down != 0);
	
	switch (keynum)
	{
	case K_SPACE:
		keydown_space = bKeyDown;
		break;

	case 'w':
		keydown_w = bKeyDown;
		break;
		
	case 's':
		keydown_s = bKeyDown;
		break;
		
	case 'a':
		keydown_a = bKeyDown;
		break;
		
	case 'd':
		keydown_d = bKeyDown;
		break;
		
	case K_CTRL:
		keydown_ctrl = bKeyDown;
		break;

	case K_SHIFT:
		keydown_shift = bKeyDown;
		break;

	case K_MOUSE1:
		keydown_mouse1 = bKeyDown;
		break;

	case K_MOUSE2:
		keydown_mouse2 = bKeyDown;
		break;

	default:
		return false;
	}

	return true;
}

void CThirdPerson::ResetRollAxis()
{
	g_Config.cvars.thirdperson_angles[2] = 0.f;
}

//-----------------------------------------------------------------------------
// Utils
//-----------------------------------------------------------------------------

void CThirdPerson::PM_NoClip(struct usercmd_s *cmd)
{
	Vector		wishvel;
	Vector		forward;
	Vector		right;
	float		fmove, smove;

	Vector vecAngles(g_Config.cvars.thirdperson_angles[0], g_Config.cvars.thirdperson_angles[1], g_Config.cvars.thirdperson_angles[2]);
	vecAngles.z = 0.f;

	AngleVectors(vecAngles, &forward, &right, NULL);

	fmove = cmd->forwardmove;
	smove = cmd->sidemove;

	for (int i = 0; i < 3; ++i)
	{
		wishvel[i] = forward[i] * fmove + right[i] * smove;
	}

	wishvel[2] += cmd->upmove;

	if (g_Config.cvars.camhack_speed_factor >= 0.0f)
		wishvel = wishvel * g_Config.cvars.camhack_speed_factor;

	VectorMA(g_Config.cvars.thirdperson_origin, Client()->Frametime(), wishvel, *reinterpret_cast<Vector *>(g_Config.cvars.thirdperson_origin));
}

void CThirdPerson::ClampViewAngles()
{
	if ( g_Config.cvars.thirdperson_angles[0] > 89.0f )
		g_Config.cvars.thirdperson_angles[0] = 89.0f;

	if ( g_Config.cvars.thirdperson_angles[0] < -89.0f )
		g_Config.cvars.thirdperson_angles[0] = -89.0f;

	if ( g_Config.cvars.thirdperson_angles[2] > 89.0f )
		g_Config.cvars.thirdperson_angles[2] = 89.0f;

	if ( g_Config.cvars.thirdperson_angles[2] < -89.0f )
		g_Config.cvars.thirdperson_angles[2] = -89.0f;
}

//-----------------------------------------------------------------------------
// Cam Hack init
//-----------------------------------------------------------------------------

CThirdPerson::CThirdPerson()
{
	m_vecViewAngles.Zero();

	m_flSavedPitchAngle = 0.f;
	m_bEditMode = false;
	m_bThirdPerson = false;

	memset(&dummy_cmd, 0, sizeof(usercmd_t));
}