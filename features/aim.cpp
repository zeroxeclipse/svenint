// Aim Feature

#include <algorithm>

#include <ICvar.h>
#include <IClient.h>
#include <IClientWeapon.h>
#include <IMemoryUtils.h>
#include <ISvenModAPI.h>
#include <convar.h>
#include <dbg.h>

#include <math/mathlib.h>
#include <hl_sdk/common/event.h>
#include <hl_sdk/engine/APIProxy.h>

#include "aim.h"
#include "misc.h"

#include "../scripts/scripts.h"

#include "../game/entitylist.h"
#include "../game/utils.h"

#include "../patterns.h"
#include "../config.h"

#define M_RAD2DEG (float)(180.0 / M_PI)
#define M_DEG2RAD (float)(M_PI / 180.0)

#define VECTOR_CONE_1DEGREES	Vector( 0.00873, 0.00873, 0.00873 )
#define VECTOR_CONE_2DEGREES	Vector( 0.01745, 0.01745, 0.01745 )
#define VECTOR_CONE_3DEGREES	Vector( 0.02618, 0.02618, 0.02618 )
#define VECTOR_CONE_4DEGREES	Vector( 0.03490, 0.03490, 0.03490 )
#define VECTOR_CONE_5DEGREES	Vector( 0.04362, 0.04362, 0.04362 )
#define VECTOR_CONE_6DEGREES	Vector( 0.05234, 0.05234, 0.05234 )
#define VECTOR_CONE_7DEGREES	Vector( 0.06105, 0.06105, 0.06105 )
#define VECTOR_CONE_8DEGREES	Vector( 0.06976, 0.06976, 0.06976 )
#define VECTOR_CONE_9DEGREES	Vector( 0.07846, 0.07846, 0.07846 )
#define VECTOR_CONE_10DEGREES	Vector( 0.08716, 0.08716, 0.08716 )
#define VECTOR_CONE_15DEGREES	Vector( 0.13053, 0.13053, 0.13053 )
#define VECTOR_CONE_20DEGREES	Vector( 0.17365, 0.17365, 0.17365 )

extern bool g_bOverrideVirtualVA;
extern bool g_bYawChanged;
extern event_t *g_pEventHooks;
extern Vector g_vecLastVirtualVA;

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

CAim g_Aim;

Vector *ev_punchangle = NULL;

EventHookFn ORIG_EventHook_FireGlock1 = NULL;
EventHookFn ORIG_EventHook_FireGlock2 = NULL;
EventHookFn ORIG_EventHook_FireShotGunSingle = NULL;
EventHookFn ORIG_EventHook_FireShotGunDouble = NULL;
EventHookFn ORIG_EventHook_FireMP5 = NULL;
EventHookFn ORIG_EventHook_FirePython = NULL;
EventHookFn ORIG_EventHook_FireDeagle = NULL;
EventHookFn ORIG_EventHook_FireGauss = NULL;
EventHookFn ORIG_EventHook_Uzi = NULL;
EventHookFn ORIG_EventHook_UziAkimbo = NULL;
EventHookFn ORIG_EventHook_WeaponCustom = NULL;
EventHookFn ORIG_EventHook_Minigun = NULL;
EventHookFn ORIG_EventHook_SniperRifle = NULL;
EventHookFn ORIG_EventHook_M249 = NULL;
EventHookFn ORIG_EventHook_M16 = NULL;
EventHookFn ORIG_EventHook_FireShockRifle = NULL;
EventHookFn ORIG_EventHook_DisplacerSpin = NULL;

//-----------------------------------------------------------------------------
// ConCommands/CVars
//-----------------------------------------------------------------------------

ConVar sc_aimbot_scripts_filter_targets( "sc_aimbot_scripts_filter_targets", "0", FCVAR_ARCHIVE, "Enable scripts callback to filter valid aimbot targets" );

CON_COMMAND( sc_aimbot, "Toggle aimbot" )
{
	bool bValue;

	if ( args.ArgC() > 1 )
	{
		bValue = !!atoi( args[ 1 ] );
	}
	else
	{
		bValue = !g_Config.cvars.aimbot;
	}

	Msg( bValue ? "Aimbot enabled\n" : "Aimbot disabled\n" );
	g_Config.cvars.aimbot = bValue;
}

CON_COMMAND( sc_aimbot_change_angles_back, "After firing with aimbot, change your viewangles back to previous one" )
{
	bool bValue;

	if ( args.ArgC() > 1 )
	{
		bValue = !!atoi( args[ 1 ] );
	}
	else
	{
		bValue = !g_Config.cvars.aimbot_change_angles_back;
	}

	Msg( bValue ? "Aimbot change angles back enabled\n" : "Aimbot change angles back disabled\n" );
	g_Config.cvars.aimbot_change_angles_back = bValue;
}

CON_COMMAND( sc_silent_aimbot, "Toggle silent aimbot" )
{
	Msg( g_Config.cvars.silent_aimbot ? "Silent Aimbot disabled\n" : "Silent Aimbot enabled\n" );
	g_Config.cvars.silent_aimbot = !g_Config.cvars.silent_aimbot;
}

CON_COMMAND( sc_ragebot, "Toggle ragebot" )
{
	Msg( g_Config.cvars.ragebot ? "Ragebot disabled\n" : "Ragebot enabled\n" );
	g_Config.cvars.ragebot = !g_Config.cvars.ragebot;
}

CON_COMMAND( sc_no_recoil, "Compensates recoil" )
{
	Msg( g_Config.cvars.no_recoil ? "No Recoil disabled\n" : "No Recoil enabled\n" );
	g_Config.cvars.no_recoil = !g_Config.cvars.no_recoil;
}

CON_COMMAND( sc_no_recoil_visual, "Removes visual effect of recoil" )
{
	Msg( g_Config.cvars.no_recoil_visual ? "No Recoil Visual disabled\n" : "No Recoil Visual enabled\n" );
	g_Config.cvars.no_recoil_visual = !g_Config.cvars.no_recoil_visual;
}

//-----------------------------------------------------------------------------
// CAim implementations
//-----------------------------------------------------------------------------

static Vector FireBullet( Vector vecSrc, Vector vecDirShooting, Vector vecSpread, int shared_rand )
{
	float x, y;
	const int iShot = 1;

	x = UTIL_SharedRandomFloat( shared_rand + iShot, -0.5, 0.5 ) + UTIL_SharedRandomFloat( shared_rand + ( 1 + iShot ), -0.5, 0.5 );
	y = UTIL_SharedRandomFloat( shared_rand + ( 2 + iShot ), -0.5, 0.5 ) + UTIL_SharedRandomFloat( shared_rand + ( 3 + iShot ), -0.5, 0.5 );

	return Vector( x * vecSpread.x, y * vecSpread.y, 0.f );
}

CAim::CAim()
{
	m_pfnV_PunchAxis = NULL;
	m_bChangeAnglesBack = false;
}

#include "../game/aim_prediction.h"

void CAim::CreateMove( float frametime, usercmd_t *cmd, int active )
{
	bool bAnglesChanged = false;

	//if ( UTIL_IsFiring(cmd) )
	//{
	//	if ( Client()->GetCurrentWeaponID() == WEAPON_GLOCK )
	//	{
	//		event_args_t args;

	//		*reinterpret_cast<Vector *>(args.origin) = g_pPlayerMove->origin;
	//		*reinterpret_cast<Vector *>(args.angles) = cmd->viewangles;
	//		*reinterpret_cast<Vector *>(args.velocity) = g_pPlayerMove->velocity;
	//		args.entindex = Client()->GetPlayerIndex();
	//		args.ducking = Client()->IsDucking();

	//		EV_FirePython( &args );
	//	}
	//}

	if ( m_bChangeAnglesBack )
	{
		cmd->viewangles = m_vecChangeAnglesTarget;
		g_pEngineFuncs->SetViewAngles( m_vecChangeAnglesTarget );

		m_bChangeAnglesBack = false;
	}

	bool bAimedToTarget = Aimbot( cmd, g_Config.cvars.aimbot, g_Config.cvars.silent_aimbot, g_Config.cvars.ragebot, g_Config.cvars.aimbot_change_angles_back, bAnglesChanged );
	
	NoRecoil( cmd );

	if ( !bAnglesChanged && g_Misc.m_bSpinnerDelayed )
	{
		g_Misc.Spinner( cmd );
	}
}

bool CAim::Aimbot( usercmd_t *cmd, bool bAimbot, bool bSilentAimbot, bool bRagebot, bool bChangeAnglesBack, bool &bAnglesChanged )
{
	WEAPON *pWeapon;
	int iWeaponID, iClip;
	bool bUsingMountedGun = IsUsingMountedGun();

	bAnglesChanged = false;

	if ( !bAimbot && !bSilentAimbot && !bRagebot )
		return false;

	// We're dead
	if ( Client()->IsDead() )
		return false;

	if ( !bUsingMountedGun )
	{
		// Don't have any weapon
		if ( ( iWeaponID = Client()->GetCurrentWeaponID() ) == WEAPON_NONE )
			return false;

		// Take out a weapon rn + exception for displacer
		if ( !Client()->CanAttack() && iWeaponID != WEAPON_DISPLACER )
			return false;

		// Don't use crowbar, medkit etc..
		if ( !IsHoldingAppropriateWeapon( iWeaponID ) )
			return false;

		// Whoops.. what happened with the weapon?
		if ( ( pWeapon = Inventory()->GetWeapon( iWeaponID ) ) == NULL )
			return false;

		// We're empty
		if ( !Inventory()->HasAmmo( pWeapon ) )
			return false;

		iClip = ClientWeapon()->Clip();

		// Going to reload
		if ( CheckReload( iWeaponID, iClip, cmd ) )
			return false;
	}

	if ( bRagebot )
	{
		if ( bUsingMountedGun )
		{
			CEntity *pTarget = FindBestTarget();

			if ( pTarget != NULL )
			{
				Vector vecTargetPoint = m_vecTargetPoint + pTarget->m_vecVelocity;
				Vector vecDir = vecTargetPoint - ( Client()->GetOrigin() + Client()->GetViewOffset() );

				Vector vAngles;
				DirectionToAngles( vecDir, vAngles );

				if ( bChangeAnglesBack )
					SetChangeAnglesBack();

				UTIL_SetAnglesSilent( vAngles, cmd );
				cmd->buttons |= IN_ATTACK;

				bAnglesChanged = true;
				return true;
			}
		}
		else
		{
			bool bStillFiring = IsStillFiring( iWeaponID, cmd );
			bool bCanPrimaryAttack = ClientWeapon()->CanPrimaryAttack();
			bool bCanSecondaryAttack = ClientWeapon()->CanSecondaryAttack();

			if ( bStillFiring || bCanPrimaryAttack || bCanSecondaryAttack )
			{
				CEntity *pTarget = FindBestTarget();

				if ( pTarget != NULL && IsTargetCanBeHurted( (eClassID)pTarget->m_classInfo.id, iWeaponID ) )
				{
					//Vector vecTargetPoint = m_vecTargetPoint + pTarget->m_vecVelocity * pTarget->m_frametime;
					Vector vecTargetPoint = m_vecTargetPoint + pTarget->m_vecVelocity;
					Vector vecDir = vecTargetPoint - ( Client()->GetOrigin() + Client()->GetViewOffset() );

					if ( bStillFiring )
					{
						Vector vAngles;
						DirectionToAngles( vecDir, vAngles );

						if ( bChangeAnglesBack )
							SetChangeAnglesBack();

						UTIL_SetAnglesSilent( vAngles, cmd );

						g_bOverrideVirtualVA = true;
						bAnglesChanged = true;

						return true;
					}
					else
					{
						float flDistance = vecDir.Length();

						if ( IsDistanceAllowsUseWeapon( iWeaponID, flDistance ) )
						{
							int fAttackButton = ConcludeAttackButton( iWeaponID, iClip, flDistance );

							if ( ( fAttackButton == IN_ATTACK && bCanPrimaryAttack ) || ( fAttackButton == IN_ATTACK2 && bCanSecondaryAttack ) )
							{
								Vector vAngles;
								DirectionToAngles( vecDir, vAngles );

								if ( bChangeAnglesBack )
									SetChangeAnglesBack();

								UTIL_SetAnglesSilent( vAngles, cmd );
								cmd->buttons |= fAttackButton;

								g_bOverrideVirtualVA = true;
								bAnglesChanged = true;

								return true;
							}
						}
					}
				}
			}
		}
	}
	else if ( bAimbot || bSilentAimbot )
	{
		if ( bUsingMountedGun && cmd->buttons & IN_ATTACK )
		{
			CEntity *pTarget = FindBestTarget();

			if ( pTarget != NULL )
			{
				Vector vecTargetPoint = m_vecTargetPoint + pTarget->m_vecVelocity;
				Vector vecDir = vecTargetPoint - ( Client()->GetOrigin() + Client()->GetViewOffset() );

				Vector vAngles;
				DirectionToAngles( vecDir, vAngles );

				if ( bSilentAimbot )
				{
					UTIL_SetAnglesSilent( vAngles, cmd );
					bAnglesChanged = true;
				}
				else
				{
					if ( bChangeAnglesBack )
						SetChangeAnglesBack();

					g_bYawChanged = true;

					cmd->viewangles = vAngles;
					g_pEngineFuncs->SetViewAngles( vAngles );
				}

				return true;
			}
		}
		else if ( IsStillFiring( iWeaponID, cmd ) || IsFiring( iWeaponID, cmd ) )
		{
			CEntity *pTarget = FindBestTarget();

			if ( pTarget != NULL && IsTargetCanBeHurted( (eClassID)pTarget->m_classInfo.id, iWeaponID ) )
			{
				//Vector vecTargetPoint = m_vecTargetPoint + pTarget->m_vecVelocity * pTarget->m_frametime;
				Vector vecTargetPoint = m_vecTargetPoint + pTarget->m_vecVelocity;
				Vector vecDir = vecTargetPoint - ( Client()->GetOrigin() + Client()->GetViewOffset() );

				float flDistance = vecDir.Length();

				if ( IsDistanceAllowsUseWeapon( iWeaponID, flDistance ) )
				{
					Vector vAngles;
					DirectionToAngles( vecDir, vAngles );

					if ( bSilentAimbot )
					{
						UTIL_SetAnglesSilent( vAngles, cmd );

						bAnglesChanged = true;
						g_bOverrideVirtualVA = true;
					}
					else
					{
						if ( bChangeAnglesBack )
							SetChangeAnglesBack();

						g_bYawChanged = true;

						cmd->viewangles = vAngles;
						g_pEngineFuncs->SetViewAngles( vAngles );
					}

					return true;
				}
			}
		}
	}

	return false;
}

void CAim::SetChangeAnglesBack()
{
	g_pEngineFuncs->GetViewAngles( m_vecChangeAnglesTarget );
	m_bChangeAnglesBack = true;
}

bool CAim::IsUsingMountedGun()
{
	extern CHud *g_pHUD;

	// offset: usermsg HideHUD
	return g_pHUD && ( *( (int *)g_pHUD + 21 ) & HIDEHUD_WEAPONS );
}

bool CAim::CheckReload( int iWeaponID, int iClip, usercmd_t *cmd )
{
	if ( ClientWeapon()->IsReloading() )
		return true;

	if ( iClip == 0 )
	{
		if ( iWeaponID == WEAPON_RPG )
		{
			// Can't reload while using laser homing
			if ( ClientWeapon()->GetWeaponData()->iuser4 && ClientWeapon()->GetWeaponData()->fuser1 != 0.f )
			{
				return false;
			}
		}

		cmd->buttons |= IN_RELOAD;
		return true;
	}

	return false;
}

bool CAim::IsHoldingAppropriateWeapon( int iWeaponID )
{
	switch ( iWeaponID )
	{
	case WEAPON_CROWBAR:
	case WEAPON_WRENCH:
	case WEAPON_MEDKIT:
	case WEAPON_HANDGRENADE:
	case WEAPON_TRIPMINE:
	case WEAPON_SATCHEL:
	case WEAPON_BARNACLE_GRAPPLE:
		return false;
	}

	return true;
}

bool CAim::IsDistanceAllowsUseWeapon( int iWeaponID, float flDistance )
{
	switch ( iWeaponID )
	{
	case WEAPON_RPG:
		if ( flDistance <= 340.f )
			return false;

		return true;

	case WEAPON_CROSSBOW:
		if ( flDistance <= 128.f )
			return false;

		return true;

	case WEAPON_EGON:
		if ( flDistance <= 128.f || flDistance > 2048.f )
			return false;

		return true;

	case WEAPON_DISPLACER:
		if ( flDistance <= 350.f )
			return false;

		return true;

	case WEAPON_SPORE_LAUNCHER:
		if ( flDistance <= 500.f || flDistance > 800.f )
			return false;

		return true;

	case WEAPON_SHOTGUN:
	case WEAPON_SNARK:
		if ( flDistance > 500.f )
			return false;

		return true;
	}

	return true;
}

bool CAim::IsTargetCanBeHurted( eClassID iClassID, int iWeaponID )
{
	switch ( iClassID )
	{
	case CLASS_NPC_TENTACLE:
	case CLASS_NPC_DESTROYED_OSPREY:
	case CLASS_NPC_SPORE_AMMO:
	case CLASS_NPC_PIT_WORM:
	case CLASS_NPC_GENEWORM:
	case CLASS_NPC_MORTAR:
		return false;

	case CLASS_NPC_GARGANTUA:
		if ( iWeaponID == WEAPON_EGON || iWeaponID == WEAPON_RPG )
			return true;

		return false;
	}

	return true;
}

int CAim::ConcludeAttackButton( int iWeaponID, int iClip, float flDistance )
{
	switch ( iWeaponID )
	{
	case WEAPON_SHOTGUN:
		if ( flDistance <= 256.f && iClip > 1 )
			return IN_ATTACK2;

		return IN_ATTACK;

	case WEAPON_HORNETGUN:
		return IN_ATTACK2;
	}

	return IN_ATTACK;
}

bool CAim::IsFiring( int iWeaponID, usercmd_t *cmd )
{
	switch ( iWeaponID )
	{
	case WEAPON_DESERT_EAGLE:
	case WEAPON_MP5:
	case WEAPON_M16:
	case WEAPON_CROSSBOW:
	case WEAPON_RPG:
	case WEAPON_EGON:
	case WEAPON_SNIPER_RIFLE:
	case WEAPON_M249:
	case WEAPON_DISPLACER:
		if ( cmd->buttons & IN_ATTACK2 )
			return false;

		break;

		/*
			case WEAPON_GAUSS:
				if ( ClientWeapon()->GetWeaponData()->fuser4 > 0.f )
				{
					if ( Client()->ButtonLast() & IN_ATTACK2 )
					{
						if ( !(cmd->buttons & IN_ATTACK2) )
							return true;
					}
					else if ( Client()->ButtonLast() & IN_ALT1 )
					{
						if ( !(cmd->buttons & IN_ALT1) )
							return true;
					}
					else if ( ClientWeapon()->GetWeaponData()->fuser4 == 1.f )
					{
						return true;
					}

					return false;
				}
				else if ( cmd->buttons & IN_ATTACK2 )
				{
					return false;
				}

				break;
		*/
	}

	if ( cmd->buttons & ( IN_ATTACK | IN_ATTACK2 ) )
	{
		if ( cmd->buttons & IN_ATTACK )
		{
			if ( ClientWeapon()->CanPrimaryAttack() )
				return true;
		}
		else
		{
			if ( ClientWeapon()->CanSecondaryAttack() )
				return true;
		}
	}

	return false;
}

bool CAim::IsStillFiring( int iWeaponID, usercmd_t *cmd )
{
	// To hit a target, we still need to aim after firing from a weapon

	switch ( iWeaponID )
	{
	case WEAPON_M16:
		if ( ClientWeapon()->GetWeaponData()->fuser2 != 0.f )
			return true;

		break;

	case WEAPON_GAUSS:
		if ( ClientWeapon()->GetWeaponData()->fuser4 > 0.f )
		{
			if ( Client()->ButtonLast() & IN_ATTACK2 )
			{
				if ( !( cmd->buttons & IN_ATTACK2 ) )
					return true;
			}
			else if ( Client()->ButtonLast() & IN_ALT1 )
			{
				if ( !( cmd->buttons & IN_ALT1 ) )
					return true;
			}
			else if ( ClientWeapon()->GetWeaponData()->fuser4 == 1.f )
			{
				return true;
			}

			return false;
		}
		else if ( cmd->buttons & IN_ATTACK2 )
		{
			return false;
		}

		break;

	case WEAPON_RPG:
		if ( ClientWeapon()->GetWeaponData()->iuser4 && ClientWeapon()->GetWeaponData()->fuser1 != 0.f )
			return true;

		break;

	case WEAPON_DISPLACER:
		if ( ClientWeapon()->GetWeaponData()->fuser1 == 1.f )
			return true;

		break;
	}

	return false;
}

CEntity *CAim::FindBestTarget()
{
	Vector va;
	Vector vForward;

	std::vector<unsigned char> vHitboxes;

	float flDistanceSqr = FLT_MAX;
	float flMaxDistanceSqr = M_SQR( g_Config.cvars.aimbot_distance );

	CEntity *pTarget = NULL;
	CEntity *pEnts = g_EntityList.GetList();

	cl_entity_t *pLocal = g_pEngineFuncs->GetLocalPlayer();
	Vector vecEyes = Client()->GetOrigin() + Client()->GetViewOffset();

	if ( g_Config.cvars.aimbot_consider_fov )
	{
		g_pEngineFuncs->GetViewAngles( va );
		AngleVectors( va, &vForward, NULL, NULL );
	}

	for ( register int i = 1; i <= g_EntityList.GetMaxEntities(); i++ )
	{
		CEntity &ent = pEnts[ i ];

		if ( !ent.m_bValid )
			continue;

		if ( ent.m_classInfo.id != CLASS_NONE )
		{
			if ( !ent.m_bEnemy )
				continue;

			if ( ent.m_bItem )
				continue;

			if ( !ent.m_bAlive )
				continue;

			if ( ent.m_classInfo.id == CLASS_NPC_SNARK || ent.m_classInfo.id == CLASS_NPC_DESTROYED_OSPREY )
				continue;
		}
		else if ( !g_Config.cvars.aimbot_aim_unknown_ents )
		{
			continue;
		}

		float dist_sqr = ( pLocal->curstate.origin - ent.m_pEntity->curstate.origin ).LengthSqr();

		if ( dist_sqr < flDistanceSqr )
		{
			Vector vecMins = ent.m_vecOrigin + ent.m_vecMins;
			Vector vecMaxs = ent.m_vecOrigin + ent.m_vecMaxs;

			Vector vecTargetPoint = ( vecMins + ( vecMaxs - vecMins ) * 0.6 );

			if ( g_Config.cvars.aimbot_consider_fov )
			{
				float angle = acos( vForward.Dot( ( vecTargetPoint - vecEyes ).Normalize() ) ) * M_RAD2DEG;

				if ( angle > g_Config.cvars.aimbot_fov )
					continue;
			}

			extra_class_info_t &extra_info = GetExtraEntityClassInfo( (eClassID)ent.m_classInfo.id );

			if ( std::binary_search( extra_info.sequence_dead.begin(), extra_info.sequence_dead.end(), (unsigned char)ent.m_pEntity->curstate.sequence ) )
				continue;

			vHitboxes.clear();

			if ( g_Config.cvars.aimbot_aim_hitboxes && !extra_info.aimbot_hitboxes.empty() )
			{
				if ( g_Config.cvars.aimbot_aim_head && std::find( vHitboxes.begin(), vHitboxes.end(), extra_info.aimbot_hitboxes[ HITBOX_HEAD ] ) == vHitboxes.end() )
					vHitboxes.push_back( extra_info.aimbot_hitboxes[ HITBOX_HEAD ] );

				if ( g_Config.cvars.aimbot_aim_neck && std::find( vHitboxes.begin(), vHitboxes.end(), extra_info.aimbot_hitboxes[ HITBOX_NECK ] ) == vHitboxes.end() )
					vHitboxes.push_back( extra_info.aimbot_hitboxes[ HITBOX_NECK ] );

				if ( g_Config.cvars.aimbot_aim_chest && std::find( vHitboxes.begin(), vHitboxes.end(), extra_info.aimbot_hitboxes[ HITBOX_CHEST ] ) == vHitboxes.end() )
					vHitboxes.push_back( extra_info.aimbot_hitboxes[ HITBOX_CHEST ] );
			}

			// No hitboxes to aim, check the mid point then
			if ( vHitboxes.empty() )
			{
				if ( IsCanSeeTarget( &ent, vecEyes, vecTargetPoint ) )
				{
					if ( sc_aimbot_scripts_filter_targets.GetBool() && !g_ScriptCallbacks.OnFilterAimbotTarget( i ) )
						continue;

					pTarget = &ent;
					flDistanceSqr = dist_sqr;
					m_vecTargetPoint = vecTargetPoint;
				}
			}
			else
			{
				for ( size_t j = 0; j < vHitboxes.size(); j++ )
				{
					vecTargetPoint = ent.m_rgHitboxes[ vHitboxes[ j ] ];

					if ( IsCanSeeTarget( &ent, vecEyes, vecTargetPoint ) )
					{
						if ( sc_aimbot_scripts_filter_targets.GetBool() && !g_ScriptCallbacks.OnFilterAimbotTarget( i ) )
							break;

						pTarget = &ent;
						flDistanceSqr = dist_sqr;
						m_vecTargetPoint = vecTargetPoint;

						break;
					}
				}
			}
		}
	}

	return pTarget;
}

bool CAim::IsCanSeeTarget( CEntity *pEntity, Vector &vecEyes, Vector &vecPoint )
{
	pmtrace_t trace;

	g_pEventAPI->EV_SetTraceHull( PM_HULL_POINT );
	g_pEventAPI->EV_PlayerTrace( vecEyes,
								 vecPoint,
								 g_Config.cvars.aimbot_ignore_blockers ? PM_WORLD_ONLY : ( g_Config.cvars.aimbot_ignore_glass ? PM_GLASS_IGNORE : PM_NORMAL ),
								 -1,
								 &trace );

	return g_Config.cvars.aimbot_ignore_blockers ? ( trace.fraction == 1.f ) : ( g_pEventAPI->EV_IndexFromTrace( &trace ) == pEntity->m_pEntity->index );
}

void CAim::DirectionToAngles( Vector &vecDir, Vector &vecAngles )
{
	vecAngles.x = -atan2f( vecDir.z, vecDir.Length2D() ) * M_RAD2DEG;
	vecAngles.y = atan2f( vecDir.y, vecDir.x ) * M_RAD2DEG;
	vecAngles.z = 0.f;
}

void CAim::NoRecoil( usercmd_t *cmd )
{
	if ( g_Config.cvars.no_recoil && !Client()->IsDead() && Client()->HasWeapon() && Client()->CanAttack() && !ClientWeapon()->IsReloading() )
	{
		// Can be wrong..

		if ( cmd->buttons & IN_ATTACK )
		{
			if ( ClientWeapon()->IsCustom() || ( !ClientWeapon()->IsCustom() && ClientWeapon()->CanPrimaryAttack() ) )
			{
				Vector vecNoRecoil;

				if ( Client()->GetCurrentWeaponID() == WEAPON_M249 )
				{
					vecNoRecoil = m_vecPunchAngle + m_vecEVPunchAngle;
				}
				else
				{
					vecNoRecoil = ( m_vecPunchAngle + m_vecEVPunchAngle ) * 2;
				}

				vecNoRecoil.z = 0.f;

				cmd->viewangles -= vecNoRecoil;

				g_bOverrideVirtualVA = true;
			}
		}
		else if ( cmd->buttons & IN_ATTACK2 )
		{
			if ( ClientWeapon()->IsCustom() || ( !ClientWeapon()->IsCustom() && ClientWeapon()->CanSecondaryAttack() ) )
			{
				Vector vecNoRecoil = ( m_vecPunchAngle + m_vecEVPunchAngle ) * 2;
				vecNoRecoil.z = 0.f;

				cmd->viewangles -= vecNoRecoil;

				g_bOverrideVirtualVA = true;
			}
		}
	}
}

void CAim::Pre_V_CalcRefdef( ref_params_t *pparams )
{
	m_vecPunchAngle = *reinterpret_cast<Vector *>( pparams->punchangle );
	m_vecEVPunchAngle = *ev_punchangle;
}

void CAim::Post_V_CalcRefdef( ref_params_t *pparams )
{
	if ( g_Config.cvars.no_recoil_visual )
	{
		*reinterpret_cast<Vector *>( pparams->viewangles ) -= m_vecPunchAngle + m_vecEVPunchAngle;
	}
}

//-----------------------------------------------------------------------------
// Event Hooks
//-----------------------------------------------------------------------------

void HOOKED_EventHook_FireGlock1( event_args_t *args )
{
	if ( g_bOverrideVirtualVA && args->entindex == Client()->GetPlayerIndex() )
		*reinterpret_cast<Vector *>( args->angles ) = g_vecLastVirtualVA;

	ORIG_EventHook_FireGlock1( args );
}

void HOOKED_EventHook_FireGlock2( event_args_t *args )
{
	if ( g_bOverrideVirtualVA && args->entindex == Client()->GetPlayerIndex() )
		*reinterpret_cast<Vector *>( args->angles ) = g_vecLastVirtualVA;

	ORIG_EventHook_FireGlock2( args );
}

void HOOKED_EventHook_FireShotGunSingle( event_args_t *args )
{
	if ( g_bOverrideVirtualVA && args->entindex == Client()->GetPlayerIndex() )
		*reinterpret_cast<Vector *>( args->angles ) = g_vecLastVirtualVA;

	ORIG_EventHook_FireShotGunSingle( args );
}

void HOOKED_EventHook_FireShotGunDouble( event_args_t *args )
{
	if ( g_bOverrideVirtualVA && args->entindex == Client()->GetPlayerIndex() )
		*reinterpret_cast<Vector *>( args->angles ) = g_vecLastVirtualVA;

	ORIG_EventHook_FireShotGunDouble( args );
}

void HOOKED_EventHook_FireMP5( event_args_t *args )
{
	if ( g_bOverrideVirtualVA && args->entindex == Client()->GetPlayerIndex() )
		*reinterpret_cast<Vector *>( args->angles ) = g_vecLastVirtualVA;

	ORIG_EventHook_FireMP5( args );
}

void HOOKED_EventHook_FirePython( event_args_t *args )
{
	if ( g_bOverrideVirtualVA && args->entindex == Client()->GetPlayerIndex() )
		*reinterpret_cast<Vector *>( args->angles ) = g_vecLastVirtualVA;

	ORIG_EventHook_FirePython( args );
}

void HOOKED_EventHook_FireDeagle( event_args_t *args )
{
	if ( g_bOverrideVirtualVA && args->entindex == Client()->GetPlayerIndex() )
		*reinterpret_cast<Vector *>( args->angles ) = g_vecLastVirtualVA;

	ORIG_EventHook_FireDeagle( args );
}

void HOOKED_EventHook_FireGauss( event_args_t *args )
{
	if ( g_bOverrideVirtualVA && args->entindex == Client()->GetPlayerIndex() )
		*reinterpret_cast<Vector *>( args->angles ) = g_vecLastVirtualVA;

	ORIG_EventHook_FireGauss( args );
}

void HOOKED_EventHook_Uzi( event_args_t *args )
{
	if ( g_bOverrideVirtualVA && args->entindex == Client()->GetPlayerIndex() )
		*reinterpret_cast<Vector *>( args->angles ) = g_vecLastVirtualVA;

	ORIG_EventHook_Uzi( args );
}

void HOOKED_EventHook_UziAkimbo( event_args_t *args )
{
	if ( g_bOverrideVirtualVA && args->entindex == Client()->GetPlayerIndex() )
		*reinterpret_cast<Vector *>( args->angles ) = g_vecLastVirtualVA;

	ORIG_EventHook_UziAkimbo( args );
}

void HOOKED_EventHook_WeaponCustom( event_args_t *args )
{
	if ( g_bOverrideVirtualVA && args->entindex == Client()->GetPlayerIndex() )
		*reinterpret_cast<Vector *>( args->angles ) = g_vecLastVirtualVA;

	ORIG_EventHook_WeaponCustom( args );
}

void HOOKED_EventHook_Minigun( event_args_t *args )
{
	if ( g_bOverrideVirtualVA && args->entindex == Client()->GetPlayerIndex() )
		*reinterpret_cast<Vector *>( args->angles ) = g_vecLastVirtualVA;

	ORIG_EventHook_Minigun( args );
}

void HOOKED_EventHook_SniperRifle( event_args_t *args )
{
	if ( g_bOverrideVirtualVA && args->entindex == Client()->GetPlayerIndex() )
		*reinterpret_cast<Vector *>( args->angles ) = g_vecLastVirtualVA;

	ORIG_EventHook_SniperRifle( args );
}

void HOOKED_EventHook_M249( event_args_t *args )
{
	if ( g_bOverrideVirtualVA && args->entindex == Client()->GetPlayerIndex() )
		*reinterpret_cast<Vector *>( args->angles ) = g_vecLastVirtualVA;

	ORIG_EventHook_M249( args );
}

void HOOKED_EventHook_M16( event_args_t *args )
{
	if ( g_bOverrideVirtualVA && args->entindex == Client()->GetPlayerIndex() )
		*reinterpret_cast<Vector *>( args->angles ) = g_vecLastVirtualVA;

	ORIG_EventHook_M16( args );
}

void HOOKED_EventHook_FireShockRifle( event_args_t *args )
{
	if ( g_bOverrideVirtualVA && args->entindex == Client()->GetPlayerIndex() )
		*reinterpret_cast<Vector *>( args->angles ) = g_vecLastVirtualVA;

	ORIG_EventHook_FireShockRifle( args );
}

void HOOKED_EventHook_DisplacerSpin( event_args_t *args )
{
	if ( g_bOverrideVirtualVA && args->entindex == Client()->GetPlayerIndex() )
		*reinterpret_cast<Vector *>( args->angles ) = g_vecLastVirtualVA;

	ORIG_EventHook_DisplacerSpin( args );
}

//-----------------------------------------------------------------------------
// Init
//-----------------------------------------------------------------------------

bool CAim::Load()
{
	m_pfnV_PunchAxis = MemoryUtils()->FindPattern( SvenModAPI()->Modules()->Client, Patterns::Client::V_PunchAxis );

	if ( !m_pfnV_PunchAxis )
	{
		Warning( "Couldn't find function \"V_PunchAxis\"\n" );
		return false;
	}

	return true;
}

void CAim::PostLoad()
{
	ud_t inst;
	MemoryUtils()->InitDisasm( &inst, m_pfnV_PunchAxis, 32, 64 );

	do
	{
		if ( inst.mnemonic == UD_Imovss && inst.operand[ 0 ].type == UD_OP_MEM && inst.operand[ 0 ].index == UD_R_EAX &&
			 inst.operand[ 0 ].scale == 4 && inst.operand[ 0 ].offset == 32 && inst.operand[ 1 ].type == UD_OP_REG && inst.operand[ 1 ].base == UD_R_XMM0 )
		{
			ev_punchangle = reinterpret_cast<Vector *>( inst.operand[ 0 ].lval.udword );
			break;
		}

	} while ( MemoryUtils()->Disassemble( &inst ) );

	event_t *pEventHook = g_pEventHooks;

	while ( pEventHook )
	{
		if ( pEventHook->name )
		{
			if ( !stricmp( pEventHook->name, "events/glock1.sc" ) )
			{
				ORIG_EventHook_FireGlock1 = pEventHook->function;
				pEventHook->function = HOOKED_EventHook_FireGlock1;
			}
			else if ( !stricmp( pEventHook->name, "events/glock2.sc" ) )
			{
				ORIG_EventHook_FireGlock2 = pEventHook->function;
				pEventHook->function = HOOKED_EventHook_FireGlock2;
			}
			else if ( !stricmp( pEventHook->name, "events/shotgun1.sc" ) )
			{
				ORIG_EventHook_FireShotGunSingle = pEventHook->function;
				pEventHook->function = HOOKED_EventHook_FireShotGunSingle;
			}
			else if ( !stricmp( pEventHook->name, "events/shotgun2.sc" ) )
			{
				ORIG_EventHook_FireShotGunDouble = pEventHook->function;
				pEventHook->function = HOOKED_EventHook_FireShotGunDouble;
			}
			else if ( !stricmp( pEventHook->name, "events/mp5.sc" ) )
			{
				ORIG_EventHook_FireMP5 = pEventHook->function;
				pEventHook->function = HOOKED_EventHook_FireMP5;
			}
			else if ( !stricmp( pEventHook->name, "events/python.sc" ) )
			{
				ORIG_EventHook_FirePython = pEventHook->function;
				pEventHook->function = HOOKED_EventHook_FirePython;
			}
			else if ( !stricmp( pEventHook->name, "events/deagle.sc" ) )
			{
				ORIG_EventHook_FireDeagle = pEventHook->function;
				pEventHook->function = HOOKED_EventHook_FireDeagle;
			}
			else if ( !stricmp( pEventHook->name, "events/gauss.sc" ) )
			{
				ORIG_EventHook_FireGauss = pEventHook->function;
				pEventHook->function = HOOKED_EventHook_FireGauss;
			}
			else if ( !stricmp( pEventHook->name, "events/uzi.sc" ) )
			{
				ORIG_EventHook_Uzi = pEventHook->function;
				pEventHook->function = HOOKED_EventHook_Uzi;
			}
			else if ( !stricmp( pEventHook->name, "events/uziakimbo.sc" ) )
			{
				ORIG_EventHook_UziAkimbo = pEventHook->function;
				pEventHook->function = HOOKED_EventHook_UziAkimbo;
			}
			else if ( !stricmp( pEventHook->name, "events/weapon_custom.sc" ) )
			{
				ORIG_EventHook_WeaponCustom = pEventHook->function;
				pEventHook->function = HOOKED_EventHook_WeaponCustom;
			}
			else if ( !stricmp( pEventHook->name, "events/minigun.sc" ) )
			{
				ORIG_EventHook_Minigun = pEventHook->function;
				pEventHook->function = HOOKED_EventHook_Minigun;
			}
			else if ( !stricmp( pEventHook->name, "events/sniperrifle.sc" ) )
			{
				ORIG_EventHook_SniperRifle = pEventHook->function;
				pEventHook->function = HOOKED_EventHook_SniperRifle;
			}
			else if ( !stricmp( pEventHook->name, "events/m249.sc" ) )
			{
				ORIG_EventHook_M249 = pEventHook->function;
				pEventHook->function = HOOKED_EventHook_M249;
			}
			else if ( !stricmp( pEventHook->name, "events/m16a2.sc" ) )
			{
				ORIG_EventHook_M16 = pEventHook->function;
				pEventHook->function = HOOKED_EventHook_M16;
			}
			//else if ( !stricmp(pEventHook->name, "events/shockrifle.sc") )
			//{
			//	ORIG_EventHook_FireShockRifle = pEventHook->function;
			//	pEventHook->function = EventHook_FireShockRifle;
			//}
			//else if ( !stricmp(pEventHook->name, "events/displacer.sc") )
			//{
			//	ORIG_EventHook_DisplacerSpin = pEventHook->function;
			//	pEventHook->function = EventHook_DisplacerSpin;
			//}

			//Msg("%s\n", pEventHook->name);
		}

		pEventHook = pEventHook->next;
	}
}

void CAim::Unload()
{
	event_t *pEventHook = g_pEventHooks;

	while ( pEventHook )
	{
		if ( pEventHook->name )
		{
			if ( !stricmp( pEventHook->name, "events/glock1.sc" ) )
			{
				pEventHook->function = ORIG_EventHook_FireGlock1;
			}
			else if ( !stricmp( pEventHook->name, "events/glock2.sc" ) )
			{
				pEventHook->function = ORIG_EventHook_FireGlock2;
			}
			else if ( !stricmp( pEventHook->name, "events/shotgun1.sc" ) )
			{
				pEventHook->function = ORIG_EventHook_FireShotGunSingle;
			}
			else if ( !stricmp( pEventHook->name, "events/shotgun2.sc" ) )
			{
				pEventHook->function = ORIG_EventHook_FireShotGunDouble;
			}
			else if ( !stricmp( pEventHook->name, "events/mp5.sc" ) )
			{
				pEventHook->function = ORIG_EventHook_FireMP5;
			}
			else if ( !stricmp( pEventHook->name, "events/python.sc" ) )
			{
				pEventHook->function = ORIG_EventHook_FirePython;
			}
			else if ( !stricmp( pEventHook->name, "events/deagle.sc" ) )
			{
				pEventHook->function = ORIG_EventHook_FireDeagle;
			}
			else if ( !stricmp( pEventHook->name, "events/gauss.sc" ) )
			{
				pEventHook->function = ORIG_EventHook_FireGauss;
			}
			else if ( !stricmp( pEventHook->name, "events/uzi.sc" ) )
			{
				pEventHook->function = ORIG_EventHook_Uzi;
			}
			else if ( !stricmp( pEventHook->name, "events/uziakimbo.sc" ) )
			{
				pEventHook->function = ORIG_EventHook_UziAkimbo;
			}
			else if ( !stricmp( pEventHook->name, "events/weapon_custom.sc" ) )
			{
				pEventHook->function = ORIG_EventHook_WeaponCustom;
			}
			else if ( !stricmp( pEventHook->name, "events/minigun.sc" ) )
			{
				pEventHook->function = ORIG_EventHook_Minigun;
			}
			else if ( !stricmp( pEventHook->name, "events/sniperrifle.sc" ) )
			{
				pEventHook->function = ORIG_EventHook_SniperRifle;
			}
			else if ( !stricmp( pEventHook->name, "events/m249.sc" ) )
			{
				pEventHook->function = ORIG_EventHook_M249;
			}
			else if ( !stricmp( pEventHook->name, "events/m16a2.sc" ) )
			{
				pEventHook->function = ORIG_EventHook_M16;
			}
			//else if ( !stricmp(pEventHook->name, "events/shockrifle.sc") )
			//{
			//	pEventHook->function = ORIG_EventHook_FireShockRifle;
			//}
			//else if ( !stricmp(pEventHook->name, "events/displacer.sc") )
			//{
			//	pEventHook->function = ORIG_EventHook_DisplacerSpin;
			//}
		}

		pEventHook = pEventHook->next;
	}
}