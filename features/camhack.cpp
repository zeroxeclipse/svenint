// Cam Hack

#include <hl_sdk/engine/APIProxy.h>

#include <ISvenModAPI.h>
#include <client_state.h>
#include <convar.h>
#include <dbg.h>
#include <keydefs.h>

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

CCamHack g_CamHack;

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

ConVar sc_camhack_attach_ignore_pitch("sc_camhack_attach_ignore_pitch", "1", FCVAR_CLIENTDLL, "Don't take into account the pitch angle of target");

CON_COMMAND_EXTERN_NO_WRAPPER(sc_camhack, ConCommand_CamHack, "Toggle CamHack")
{
	if ( !Client()->IsInGame() || Client()->IsSpectating() )
		return;

	if (g_CamHack.IsEnabled())
	{
		Msg("CamHack disabled\n");
		g_CamHack.Disable();
	}
	else
	{
		Msg("CamHack enabled\n");
		g_CamHack.Enable();
	}
}

CON_COMMAND(sc_camhack_attach, "Attach camera to player")
{
	if ( !Client()->IsInGame() )
		return;

	if ( args.ArgC() >= 2 )
	{
		int index = atoi( args[1] );

		if (index > 0 && index <= MAXCLIENTS)
		{
			g_CamHack.Attach( index );
		}
		else
		{
			g_CamHack.Deattach();
		}
	}
	else
	{
		Msg("Usage:  sc_camhack_attach <player index>\n");
	}
}

CON_COMMAND(sc_camhack_deattach, "Deattach camera from player")
{
	g_CamHack.Deattach();
}

CON_COMMAND_EXTERN_NO_WRAPPER(sc_camhack_reset_roll, ConCommand_CamHackResetRoll, "Reset camera's roll axis to zero")
{
	if (g_CamHack.IsEnabled())
	{
		g_CamHack.ResetRollAxis();
	}
}

CON_COMMAND_EXTERN_NO_WRAPPER(sc_camhack_reset, ConCommand_CamHackReset, "Teleport to your original position")
{
	if (g_CamHack.IsEnabled())
	{
		g_CamHack.ResetOrientation();
	}
}

//-----------------------------------------------------------------------------
// Cam Hack
//-----------------------------------------------------------------------------

void CCamHack::CreateMove(float frametime, struct usercmd_s *cmd, int active)
{
	if (m_bEnabled || m_bAttached)
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
			m_vecCameraAngles.z -= 0.2f;

		if (keydown_mouse2)
			m_vecCameraAngles.z += 0.2f;

		dummy_cmd.upmove *= 0.75f;

		Vector va_delta = g_newviewangles - g_oldviewangles;

		// ToDo: for better rotation, use quaternions when the camera is tilted
		m_vecCameraAngles += va_delta;

		NormalizeAngles( m_vecCameraAngles );
		ClampViewAngles( m_vecCameraAngles );

		PM_NoClip( &dummy_cmd );

		cmd->viewangles = bAnglesChanged ? g_vecSpinAngles : m_vecViewAngles;

		cmd->forwardmove = 0.f;
		cmd->sidemove = 0.f;

		cmd->buttons &= ~(0xFFFFFFFF & ~IN_DUCK);
	}
}

void CCamHack::V_CalcRefdef(struct ref_params_s *pparams)
{
	if ( m_bEnabled )
	{
		if ( SvenModAPI()->GetClientState() == CLS_ACTIVE && !Client()->IsSpectating() )
		{
			cl_entity_t *pLocal = g_pEngineFuncs->GetLocalPlayer();

			*reinterpret_cast<Vector *>(pparams->vieworg) = m_vecCameraOrigin;
			*reinterpret_cast<Vector *>(pparams->viewangles) = m_vecCameraAngles;

			pLocal->angles.x = m_flSavedPitchAngle;
			pLocal->curstate.angles.x = m_flSavedPitchAngle;
			pLocal->prevstate.angles.x = m_flSavedPitchAngle;
			pLocal->latched.prevangles.x = m_flSavedPitchAngle;
		}
		else
		{
			Disable();
		}
	}

	if ( m_bAttached )
	{
		cl_entity_t *pLocal = g_pEngineFuncs->GetLocalPlayer();
		cl_entity_t *pEntity = g_pEngineFuncs->GetEntityByIndex( m_iAttachTarget );

		if ( SvenModAPI()->GetClientState() == CLS_ACTIVE && pEntity && pEntity->curstate.messagenum >= pLocal->curstate.messagenum )
		{
			float localToWorld[3][4];
			Vector vecOrigin, vecAngles, vecForward, tmp;
			Vector vecTargetAngles = pEntity->angles;

			if ( sc_camhack_attach_ignore_pitch.GetBool() )
				vecTargetAngles.x = 0.f;
			else
				vecTargetAngles.z *= (-89.0f / 9.8876953125f);

			vecTargetAngles.z = 0.f;

			AngleVectors( m_vecCameraAngles, &vecForward, NULL, NULL );
			AngleMatrix( vecTargetAngles, localToWorld );

			VectorTransform( vecForward, localToWorld, tmp );

			localToWorld[0][3] = pEntity->origin.x;
			localToWorld[1][3] = pEntity->origin.y;
			localToWorld[2][3] = pEntity->origin.z;

			VectorTransform( m_vecCameraOrigin, localToWorld, vecOrigin );

			vecAngles.x = -atan2f(tmp.z, tmp.Length2D()) * (float)(180.0 / M_PI);
			vecAngles.y = atan2f(tmp.y, tmp.x) * (float)(180.0 / M_PI);
			vecAngles.z = 0.f;

			*reinterpret_cast<Vector *>(pparams->vieworg) = vecOrigin;
			*reinterpret_cast<Vector *>(pparams->viewangles) = vecAngles;

			pLocal->angles.x = m_flSavedPitchAngle;
			pLocal->curstate.angles.x = m_flSavedPitchAngle;
			pLocal->prevstate.angles.x = m_flSavedPitchAngle;
			pLocal->latched.prevangles.x = m_flSavedPitchAngle;
		}
		else
		{
			Disable();
		}
	}
}

void CCamHack::Enable()
{
	m_bEnabled = true;
	m_bAttached = false;

	m_iAttachTarget = 0;

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

	m_vecCameraOrigin = g_pPlayerMove->origin + Client()->GetViewOffset();
	m_vecCameraAngles = m_vecViewAngles;

	m_flSavedPitchAngle = NormalizeAngle(m_vecViewAngles.x) / -3.0f;

	if (g_Config.cvars.camhack_hide_hud)
	{
		CVar()->SetValue(hud_draw, 0);
	}

	if (g_Config.cvars.camhack_show_model)
	{
		if ( !g_pClientFuncs->CL_IsThirdPerson() )
		{
			g_pEngineFuncs->ClientCmd("thirdperson\n");
			m_bChangeToThirdPerson = false;
		}
		else
		{
			m_bChangeToThirdPerson = true;
		}

		m_bChangeCameraState = true;
	}
	else
	{
		if ( g_pClientFuncs->CL_IsThirdPerson() )
			g_pEngineFuncs->ClientCmd("firstperson\n");

		m_bChangeCameraState = false;
	}
}

void CCamHack::Disable()
{
	m_bEnabled = false;
	m_bAttached = false;

	m_iAttachTarget = 0;

	keydown_w = false;
	keydown_s = false;
	keydown_a = false;
	keydown_d = false;
	keydown_space = false;
	keydown_ctrl = false;
	keydown_shift = false;
	keydown_mouse1 = false;
	keydown_mouse2 = false;

	if (g_Config.cvars.camhack_hide_hud)
	{
		CVar()->SetValue(hud_draw, 1);
	}

	if (m_bChangeCameraState)
	{
		if (m_bChangeToThirdPerson)
		{
			g_pEngineFuncs->ClientCmd("thirdperson\n");
		}
		else
		{
			g_pEngineFuncs->ClientCmd("firstperson\n");
		}
	}

	g_pEngineFuncs->SetViewAngles(m_vecViewAngles);

	m_bChangeCameraState = false;
}

void CCamHack::Attach(int playerIndex)
{
	float worldToLocal[3][4];
	cl_entity_t *pEntity = NULL;

	if ( playerIndex < 1 || playerIndex > MAXCLIENTS )
		return;

	if ( SvenModAPI()->GetClientState() != CLS_ACTIVE ||
		(pEntity = g_pEngineFuncs->GetEntityByIndex(playerIndex)) == NULL ||
		pEntity->curstate.messagenum < g_pEngineFuncs->GetLocalPlayer()->curstate.messagenum )
		return;

	m_bEnabled = false;
	m_bAttached = true;

	m_iAttachTarget = playerIndex;

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

	// Render origin & angles
	Vector vecTargetOrigin = pEntity->origin;
	Vector vecTargetAngles = pEntity->angles;

	Vector vecForward, tmp;
	Vector vecOrigin = g_pPlayerMove->origin + Client()->GetViewOffset();

	vecTargetAngles.x = 0.f;
	vecTargetAngles.z = 0.f;

	AngleIMatrix( vecTargetAngles, worldToLocal );
	AngleVectors( m_vecViewAngles, &vecForward, NULL, NULL );

	// Translate origin to local space
	vecOrigin.x -= vecTargetOrigin.x;
	vecOrigin.y -= vecTargetOrigin.y;
	vecOrigin.z -= vecTargetOrigin.z;

	VectorTransform( vecOrigin, worldToLocal, m_vecCameraOrigin );

	// Translate angles to local space
	VectorTransform( vecForward, worldToLocal, tmp );

	m_vecCameraAngles.x = -atan2f(tmp.z, tmp.Length2D()) * (float)(180.0 / M_PI);
	m_vecCameraAngles.y = atan2f(tmp.y, tmp.x) * (float)(180.0 / M_PI);
	m_vecCameraAngles.z = 0.f;


	if (g_Config.cvars.camhack_hide_hud)
	{
		CVar()->SetValue(hud_draw, 0);
	}

	if (g_Config.cvars.camhack_show_model)
	{
		if ( !g_pClientFuncs->CL_IsThirdPerson() )
		{
			g_pEngineFuncs->ClientCmd("thirdperson\n");
			m_bChangeToThirdPerson = false;
		}
		else
		{
			m_bChangeToThirdPerson = true;
		}

		m_bChangeCameraState = true;
	}
	else
	{
		if ( g_pClientFuncs->CL_IsThirdPerson() )
			g_pEngineFuncs->ClientCmd("firstperson\n");

		m_bChangeCameraState = false;
	}
}

void CCamHack::Deattach()
{
	Disable();
}

bool CCamHack::StudioRenderModel()
{
	if ( (m_bEnabled || m_bAttached) && !g_pClientFuncs->CL_IsThirdPerson() && g_pStudioRenderer->m_pCurrentEntity == g_pEngineFuncs->GetViewModel() )
		return true;

	return false;
}

void CCamHack::OnVideoInit()
{
	if (m_bEnabled || m_bAttached)
	{
		Disable();
	}
}

bool CCamHack::OnKeyPress(int down, int keynum)
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

void CCamHack::ResetRollAxis()
{
	m_vecCameraAngles.z = 0.0f;
}

void CCamHack::ResetOrientation()
{
	m_vecCameraOrigin = g_pPlayerMove->origin + Client()->GetViewOffset();
}

//-----------------------------------------------------------------------------
// Utils
//-----------------------------------------------------------------------------

void CCamHack::PM_NoClip(struct usercmd_s *cmd)
{
	Vector		wishvel;
	Vector		forward;
	Vector		right;
	float		fmove, smove;

	Vector vecAngles = m_vecCameraAngles;
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

	VectorMA(m_vecCameraOrigin, Client()->Frametime(), wishvel, m_vecCameraOrigin);
}

void CCamHack::ClampViewAngles(Vector &viewangles)
{
	if (viewangles[0] > 89.0f)
		viewangles[0] = 89.0f;

	if (viewangles[0] < -89.0f)
		viewangles[0] = -89.0f;

	if (viewangles[2] > 89.0f)
		viewangles[2] = 89.0f;

	if (viewangles[2] < -89.0f)
		viewangles[2] = -89.0f;
}

//-----------------------------------------------------------------------------
// Cam Hack init
//-----------------------------------------------------------------------------

CCamHack::CCamHack()
{
	m_bEnabled = false;
	m_bAttached = false;

	m_iAttachTarget = 0;

	m_flSavedPitchAngle = 0.0f;

	m_bChangeCameraState = false;
	m_bChangeToThirdPerson = false;

	m_vecCameraOrigin = { 0.0f, 0.0f, 0.0f };
	m_vecCameraAngles = { 0.0f, 0.0f, 0.0f };

	m_vecViewAngles = { 0.0f, 0.0f, 0.0f };

	memset(&dummy_cmd, 0, sizeof(usercmd_t));
}

void CCamHack::Init()
{
}