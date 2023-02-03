// Anti-AFK

#include <hl_sdk/engine/APIProxy.h>

#include <IInventory.h>
#include <IClient.h>
#include <convar.h>
#include <dbg.h>

#include "antiafk.h"

#include "../game/utils.h"
#include "../config.h"

extern float g_flClientDataLastUpdate;
extern bool g_bLoading;

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

CAntiAFK g_AntiAFK;

static bool s_bSpawning = false;

UserMsgHookFn ORIG_UserMsgHook_Health = NULL;

//-----------------------------------------------------------------------------
// ConCommands
//-----------------------------------------------------------------------------

CON_COMMAND_EXTERN(sc_antiafk, ConCommand_AntiAFK, "Set Anti-AFK Mode [0-5]. Available modes:\n0 - Off\n1 - Step Forward & Back\n2 - Spam Gibme\n3 - Spam Kill\n4 - Walk Around & Spam Inputs\n5 - Walk Around\n6 - Go Right")
{
	if (args.ArgC() >= 2)
	{
		int nMode = atoi(args[1]);

		if (nMode >= 0 && nMode <= 5)
		{
			g_Config.cvars.antiafk = nMode;
		}
		else
		{
			Msg("Wrong Anti-AFK mode\n");
		}
	}
	else
	{
		Msg("Usage:  sc_antiafk <mode>\n");
	}
}

//-----------------------------------------------------------------------------
// Usermsg Health
//-----------------------------------------------------------------------------

int UserMsgHook_Health(const char *pszUserMsg, int iSize, void *pBuffer)
{
	auto result = ORIG_UserMsgHook_Health(pszUserMsg, iSize, pBuffer);

	g_AntiAFK.OnRespawn();

	return result;
}

//-----------------------------------------------------------------------------
// Anti-AFK implementations
//-----------------------------------------------------------------------------

CAntiAFK::CAntiAFK()
{
	Reset();

	m_bWaitingForClientdata = false;
	m_bWaitingForRespawn = false;
}

void CAntiAFK::Reset()
{
	m_bDead = false;
	m_bComingBackToAFKPoint = false;

	m_vecAFKPoint.x = 0.f;
	m_vecAFKPoint.y = 0.f;

	m_flComingBackStartTime = -1.f;
}

void CAntiAFK::OnVideoInit()
{
	Reset();

	//m_bWaitingForClientdata = true;
	//m_bWaitingForRespawn = false;
	m_bWaitingForRespawn = true;
}

void CAntiAFK::CreateMove(float frametime, struct usercmd_s *cmd, int active)
{
	AntiAFK(cmd);
}

void CAntiAFK::OnEndLoading()
{
	//m_bWaitingForClientdata = true;

	OnDie();
}

void CAntiAFK::OnEnterToServer()
{
	//if ( m_bWaitingForClientdata )
	//{
	//	m_vecAFKPoint.x = g_pPlayerMove->origin.x;
	//	m_vecAFKPoint.y = g_pPlayerMove->origin.y;

	//	m_bDead = true;
	//	m_bComingBackToAFKPoint = false;

	//	m_flComingBackStartTime = -1.f;

	//	m_bWaitingForClientdata = false;
	//}
}

void CAntiAFK::OnRespawn()
{
	if ( m_bWaitingForRespawn )
	{
		m_vecAFKPoint.x = g_pPlayerMove->origin.x;
		m_vecAFKPoint.y = g_pPlayerMove->origin.y;

		m_bDead = false;
		m_bComingBackToAFKPoint = false;

		m_flComingBackStartTime = -1.f;

		m_bWaitingForRespawn = false;
	}
}

void CAntiAFK::OnDie()
{
	m_bDead = true;
	m_bComingBackToAFKPoint = false;

	m_vecAFKPoint.x = 0.f;
	m_vecAFKPoint.y = 0.f;

	m_flComingBackStartTime = -1.f;

	m_bWaitingForRespawn = true;
}

void CAntiAFK::AntiAFK(struct usercmd_s *cmd)
{
	int nMode = g_Config.cvars.antiafk;

	if ( !nMode )
	{
		Reset();
		return;
	}

	bool bDead = Client()->IsDead();

	if ( m_bDead != bDead )
	{
		if ( bDead )
		{
			OnDie();
		}
	}

	if ( bDead )
		return;

	if ( g_Config.cvars.antiafk_stay_within_range && !(nMode == 2 || nMode == 3 ))
	{
		if ( m_bWaitingForRespawn || m_bWaitingForClientdata )
			return;

		if ( m_vecAFKPoint.x == 0.0f && m_vecAFKPoint.y == 0.0f )
			m_vecAFKPoint = g_pPlayerMove->origin.AsVector2D();

		Vector2D vecOrigin = g_pPlayerMove->origin.AsVector2D();
		float flDistanceToAFKPointSqr = (m_vecAFKPoint - vecOrigin).LengthSqr();

		if ( m_bComingBackToAFKPoint || flDistanceToAFKPointSqr > M_SQR(g_Config.cvars.antiafk_stay_radius) ) // moved out of range
		{
			bool bReset = false;

			if ( m_flComingBackStartTime == -1.0f )
			{
				m_flComingBackStartTime = g_pEngineFuncs->GetClientTime();
			}
			else if ( g_Config.cvars.antiafk_reset_stay_pos && g_pEngineFuncs->GetClientTime() - m_flComingBackStartTime >= 10.f )
			{
				// Coming to the AFK point too long, reset current state
				Reset();
				bReset = true;
			}
			
			if ( !bReset )
			{
				if ( m_bComingBackToAFKPoint && flDistanceToAFKPointSqr <= M_SQR(25.0f) )
				{
					m_bComingBackToAFKPoint = false;
					m_flComingBackStartTime = -1.0f;
					return;
				}

				Vector2D vecForward;
				Vector2D vecRight;

				Vector2D vecDir = m_vecAFKPoint - vecOrigin;

				m_bComingBackToAFKPoint = true;
				vecDir.NormalizeInPlace();

				// Rotate the wish vector by a random direction, must help if we stuck somewhere
				int nRandom = g_pEngineFuncs->RandomLong(0, 1);

				float flTheta = g_Config.cvars.antiafk_stay_radius_offset_angle * static_cast<float>(M_PI) / 180.0f;

				float ct = cosf(flTheta);
				float st = sinf(flTheta);

				if (nRandom == 1) // counter clockwise
				{
					vecDir.x = vecDir.x * ct - vecDir.y * st;
					vecDir.y = vecDir.x * st + vecDir.y * ct;
				}
				else // clockwise
				{
					vecDir.x = vecDir.x * ct + vecDir.y * st;
					vecDir.y = -vecDir.x * st + vecDir.y * ct;
				}

				// Forward angles
				vecForward.x = cosf(cmd->viewangles.y * static_cast<float>(M_PI) / 180.f);
				vecForward.y = sinf(cmd->viewangles.y * static_cast<float>(M_PI) / 180.f);

				// Make a right vector of angles. Rotate forward vector as a complex number by 90 deg.
				vecRight.x = vecForward.y;
				vecRight.y = -vecForward.x;

				// Multiply by max movement speed
				vecForward *= Client()->GetMaxSpeed();
				vecRight *= Client()->GetMaxSpeed();

				// Project onto direction vector
				float forwardmove = DotProduct(vecForward, vecDir);
				float sidemove = DotProduct(vecRight, vecDir);

				// Apply moves
				cmd->forwardmove = forwardmove;
				cmd->sidemove = sidemove;

				return;
			}
		}
	}
	else
	{
		m_bComingBackToAFKPoint = false;
		m_flComingBackStartTime = -1.0f;
	}

	switch (nMode)
	{
	case 1:
	{
		static bool forward_step = true;

		cmd->forwardmove = forward_step ? 50.0f : -50.0f;

		forward_step = !forward_step;

		break;
	}

	case 2:
	case 3:
	{
		bool bSuicided = false;
		bool bHasAnyWeapon = false;

		switch ( Client()->GetCurrentWeaponID() )
		{
		case WEAPON_CROWBAR:
		case WEAPON_MEDKIT:
		case WEAPON_WRENCH:
		case WEAPON_BARNACLE_GRAPPLE:
			if (nMode == 2)
				g_pEngineFuncs->ClientCmd("gibme\n");
			else
				g_pEngineFuncs->ClientCmd("kill\n");

			bSuicided = true;
			break;
		}

		if ( !bSuicided )
		{
			bool bFound = false;
			WEAPON *pWeapon = NULL;

			for (int i = 0; i < Inventory()->GetMaxWeaponSlots(); i++)
			{
				for (int j = 0; j < Inventory()->GetMaxWeaponPositions(); j++)
				{
					if (pWeapon = Inventory()->GetWeapon(i, j))
					{
						if ( !bHasAnyWeapon && Inventory()->HasAmmo(pWeapon) )
							bHasAnyWeapon = true;

						switch (pWeapon->iId)
						{
						case WEAPON_CROWBAR:
						case WEAPON_WRENCH:
						case WEAPON_BARNACLE_GRAPPLE:
							Inventory()->SelectWeapon(pWeapon);
							bFound = true;

							break;

						case WEAPON_MEDKIT:
							if (Inventory()->GetPrimaryAmmoCount(pWeapon) > 0)
							{
								Inventory()->SelectWeapon(pWeapon);
								bFound = true;
							}

							break;
						}

						if (bFound)
							break;
					}
				}

				if (bFound)
					break;
			}

			if ( !bFound )
			{
				if (bHasAnyWeapon)
				{
					if ( !(Client()->GetCurrentWeaponID() == WEAPON_NONE && (Client()->Time() - g_flClientDataLastUpdate) >= 0.5f) )
						g_pEngineFuncs->ClientCmd("gibme\n");
				}
				else if (nMode == 2)
				{
					g_pEngineFuncs->ClientCmd("gibme\n");
				}
				else
				{
					g_pEngineFuncs->ClientCmd("kill\n");
				}
			}
		}

		break;
	}

	case 4:
	{
		constexpr int delay_count = 30;

		static int delay = 0;
		static int attack_button_idx = 0;
		static int attack_button = IN_ATTACK;

		static const int attack_buttons[] =
		{
			IN_ATTACK,
			IN_JUMP,
			IN_DUCK,
			IN_USE,
			IN_CANCEL,
			IN_LEFT,
			IN_RIGHT,
			IN_ATTACK2,
			IN_RUN,
			IN_RELOAD
			// IN_ALT1
		};

		++delay;

		if (delay > delay_count)
		{
			if (attack_button_idx == (sizeof(attack_buttons) / sizeof(*attack_buttons)))
				attack_button_idx = 0;

			attack_button = attack_buttons[attack_button_idx++];

			delay = 0;
		}

		cmd->buttons |= attack_button;

		{
			constexpr int delay_count = 60;

			static int delay = 0;
			static int movement_button = IN_MOVERIGHT;

			WalkAround(cmd, delay, movement_button, delay_count);

			RotateCamera();
		}

		break;
	}

	case 5:
	{
		constexpr int delay_count = 60;

		static int delay = 0;
		static int movement_button = IN_MOVERIGHT;

		WalkAround(cmd, delay, movement_button, delay_count);

		RotateCamera();

		break;
	}

	case 6:
	{
		cmd->buttons |= IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT;
		cmd->sidemove = Client()->GetMaxSpeed();

		RotateCamera();

		break;
	}
	}

	if (Client()->GetWaterLevel() == WL_EYES)
	{
		cmd->upmove = Client()->GetMaxSpeed();
	}
}

void CAntiAFK::RotateCamera()
{
	if (g_Config.cvars.antiafk_rotate_camera)
	{
		Vector va;

		float flRotationAngle = g_Config.cvars.antiafk_rotation_angle;
		float flRotationAngleAbs = fabsf(flRotationAngle);

		static float s_flPitchDirection = 1.0f;

		g_pEngineFuncs->GetViewAngles(va);

		if (s_flPitchDirection > 0.0f)
		{
			if (va.x + flRotationAngleAbs > 89.0f)
				s_flPitchDirection = -1.0f;
			else
				va.x += flRotationAngleAbs;
		}
		else if (s_flPitchDirection < 0.0f)
		{
			if (va.x - flRotationAngleAbs < -89.0f)
				s_flPitchDirection = 1.0f;
			else
				va.x -= flRotationAngleAbs;
		}

		va.y += flRotationAngle;
		va.y = NormalizeAngle(va.y);

		g_pEngineFuncs->SetViewAngles(va);
	}
}

void CAntiAFK::WalkAround(struct usercmd_s *cmd, int &delay, int &movement_button, const int delay_count)
{
REPEAT:
	if (movement_button == IN_MOVERIGHT)
	{
		if (delay > delay_count)
		{
			movement_button = IN_BACK;
			delay = 0;

			goto REPEAT;
		}
		else
		{
			cmd->sidemove = Client()->GetMaxSpeed();
		}
	}
	else if (movement_button == IN_BACK)
	{
		if (delay > delay_count)
		{
			movement_button = IN_MOVELEFT;
			delay = 0;

			goto REPEAT;
		}
		else
		{
			cmd->forwardmove = -Client()->GetMaxSpeed();
		}
	}
	else if (movement_button == IN_MOVELEFT)
	{
		if (delay > delay_count)
		{
			movement_button = IN_FORWARD;
			delay = 0;

			goto REPEAT;
		}
		else
		{
			cmd->sidemove = -Client()->GetMaxSpeed();
		}
	}
	else if (movement_button == IN_FORWARD)
	{
		if (delay > delay_count)
		{
			movement_button = IN_MOVERIGHT;
			delay = 0;

			goto REPEAT;
		}
		else
		{
			cmd->forwardmove = Client()->GetMaxSpeed();
		}
	}

	cmd->buttons |= movement_button;
	++delay;
}

//-----------------------------------------------------------------------------
// Anti-AFK feature
//-----------------------------------------------------------------------------

bool CAntiAFK::Load()
{
	return true;
}

void CAntiAFK::PostLoad()
{
	m_hUserMsgHook_Health = Hooks()->HookUserMessage( "Health", UserMsgHook_Health, &ORIG_UserMsgHook_Health );
}

void CAntiAFK::Unload()
{
	Hooks()->UnhookUserMessage( m_hUserMsgHook_Health );
}