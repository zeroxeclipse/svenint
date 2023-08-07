// Speedrun Tools

#include <vector>
#include <algorithm>

#include <ISvenModAPI.h>
#include <IMemoryUtils.h>
#include <IHooks.h>

#include <convar.h>
#include <dbg.h>

#include <messagebuffer.h>

#include "speedrun_tools.h"
#include "input_manager.h"

#include "../modules/opengl.h"
#include "../modules/server.h"
#include "../modules/server_client_bridge.h"

#include "../game/draw_context.h"
#include "../game/drawing.h"
#include "../game/utils.h"
#include "../game/entitylist.h"
#include "../game/structs.h"
#include "../game/demo_message.h"

#include "../strafe/strafe_utils.h"
#include "../scripts/scripts.h"
#include "../scripts/lua_entity_dictionary.h"

#include "../patterns.h"
#include "../config.h"

extern bool g_bPlayingbackDemo;
extern ref_params_t refparams;
extern movevars_t refparams_movevars;

//-----------------------------------------------------------------------------
// Declare Hooks... and function pointer
//-----------------------------------------------------------------------------

DECLARE_FUNC_PTR( vgui::HFont, __cdecl, VGUI2_GetEngineFont );

DECLARE_CLASS_HOOK( void, CBaseEntity_FireBullets, void *thisptr, unsigned int, Vector, Vector, Vector, float, int, int, int, entvars_t *, int );
DECLARE_CLASS_HOOK( void, CFuncTankGun_Fire, void *thisptr, const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker );
DECLARE_HOOK( void, __cdecl, UTIL_GetCircularGaussianSpread, float *, float * );
DECLARE_HOOK( qboolean, __cdecl, Host_FilterTime, float );

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

DEFINE_PATTERN( CBaseEntity_FireBullets_sig, "55 8B EC 6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 81 EC ? ? ? ? 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 ? ? ? ? 8B F9 89 7D F0" );
DEFINE_PATTERN( CFuncTankGun_Fire_sig, "53 55 56 8B F1 57 F3 0F 10 86 A0 01 00 00" );
DEFINE_PATTERN( UTIL_GetCircularGaussianSpread_sig, "56 8B 74 24 08 57 8B 7C 24 10 66 0F 1F 44 00 00" );
DEFINE_PATTERN( Host_FilterTime_sig, "E9 ? ? ? ? 90 90 90 8B 0D ? ? ? ? D8" );
DEFINE_PATTERN( host_framerate_patch_sig, "74 ? DD ? B8" );

CSpeedrunTools g_SpeedrunTools;

// sc_st_setangles
static bool s_bSetAngles = false;
static bool s_bSetAngles2 = false;
static Vector s_vecSetAngles;
static Vector s_vecSetAnglesSpeed;
static Vector s_vecSetAngles2;
static float s_flSetAngles2Lerp;
// sc_st_follow_point
static bool s_bFollowPoint = false;
static float s_flFollowPointLerp = 1.f;
static Vector s_vecFollowPoint;

bool is_hl_c17 = false;
int iNihilanthIndex = 0;
entvars_t *pNihilanthVars = NULL;

// cvars
static cvar_t *fps_max = NULL;
static cvar_t *host_framerate = NULL;
static bool s_bIgnoreCvarChange = false;
static bool s_bNotifyTimescaleChanged = false;

//-----------------------------------------------------------------------------
// ConCommands, CVars..
//-----------------------------------------------------------------------------

ConVar sc_st_min_frametime( "sc_st_min_frametime", "0", FCVAR_CLIENTDLL, "Min frametime to run a frame" );

ConVar sc_st_map_start_position( "sc_st_map_start_position", "", FCVAR_CLIENTDLL, "Restart map if player hasn't spawned in the given position\n\"x y\" - 2D position\n\"\" - disabled" );
ConVar sc_st_disable_spread( "sc_st_disable_spread", "0", FCVAR_CLIENTDLL, "Disables spread" );

ConVar sc_st_legit_mode_ignore_freeze( "sc_st_legit_mode_ignore_freeze", "0", FCVAR_CLIENTDLL, "Don't block freeze of the host when legit mode is on" );
ConVar sc_st_legit_mode_block_freeze_mouse_input( "sc_st_legit_mode_block_freeze_mouse_input", "1", FCVAR_CLIENTDLL, "When frozen, disabled mouse input" );

static void CvarChangeHook_fps_max( cvar_t *pCvar, const char *pszOldValue, float flOldValue )
{
	if ( s_bIgnoreCvarChange || flOldValue == pCvar->value )
		return;

	if ( sc_st_min_frametime.GetFloat() != 0.f )
	{
		float timescale = 1.f / ( sc_st_min_frametime.GetFloat() * flOldValue );
		float multiplier = 1.f / timescale;

		sc_st_min_frametime.SetValue( multiplier / pCvar->value );
		CVar()->SetValue( CVar()->FindCvar( "host_framerate" ), 1.f / pCvar->value );

		Utils()->PrintChatText( "<SvenInt> Automatically adjusted timescale %.2f for %d fps\n", timescale, (int)pCvar->value );
	}
}

CON_COMMAND_NO_WRAPPER( sc_st_timer, "Show speedrun timer" )
{
	Msg( g_Config.cvars.st_timer ? "Speedrun Timer disabled\n" : "Speedrun Timer enabled\n" );
	g_Config.cvars.st_timer = !g_Config.cvars.st_timer;
}

CON_COMMAND_NO_WRAPPER( sc_st_reset_timer, "Reset speedrun timer" )
{
	g_SpeedrunTools.StartTimer();
}

CON_COMMAND_NO_WRAPPER( sc_st_legit_mode, "Forcibly disable all not legit cheats / features in SvenInt" )
{
	Msg( "<SvenInt> Legit mode is ON\n" );
	Utils()->PrintChatText( "<SvenInt> Legit mode is ON" );

	g_Config.cvars.ragebot = false;
	g_Config.cvars.silent_aimbot = false;
	g_Config.cvars.fakelag = false;
	g_Config.cvars.fast_crowbar = false;
	g_Config.cvars.fast_crowbar2 = false;
	g_Config.cvars.fast_medkit = false;
	g_Config.cvars.rotate_dead_body = false;
	g_Config.cvars.quake_guns = false;
	g_Config.cvars.no_recoil = false;
	g_Config.cvars.fp_roaming = false;
	g_Config.cvars.thirdperson = false;
	g_Config.cvars.one_tick_exploit = false;
	g_Config.cvars.replace_players_models = false;
	g_Config.cvars.replace_model_on_self = false;
	g_Config.cvars.replace_players_models_with_randoms = false;
	g_Config.cvars.replace_specified_players_models = false;
	g_Config.cvars.revert_pitch = false;
	g_Config.cvars.revert_yaw = false;
	g_Config.cvars.spin_pitch_angle = false;
	g_Config.cvars.lock_pitch = false;
	g_Config.cvars.lock_yaw = false;
	g_Config.cvars.skybox = 0;

	g_SpeedrunTools.SetLegitMode( true );
}

CON_COMMAND( sc_st_timescale, "Set timescale" )
{
	if ( args.ArgC() >= 2 )
	{
		g_SpeedrunTools.SetTimescale( atof( args[ 1 ] ) );
		s_bNotifyTimescaleChanged = true;
	}
	else
	{
		if ( sc_st_min_frametime.GetFloat() != 0.f )
			Msg( "Current timescale: %.3f\n", 1.f / ( sc_st_min_frametime.GetFloat() * CVar()->FindCvar( "fps_max" )->value ) );
		else
			Msg( "Current timescale: 1.000\n" );
	}
}

CON_COMMAND( sc_st_obsclip, "Simulate observer clipping: sc_st_obsclip <speed> <fps>" )
{
	if ( args.ArgC() >= 3 && Host_IsServerActive() )
	{
		edict_t *pPlayer = g_pServerEngineFuncs->pfnPEntityOfEntIndex( g_pPlayerMove->player_index + 1 );

		if ( pPlayer == NULL )
			return;

		Vector va, vecSimOrigin, vecMove;

		Vector vecMins = ( Client()->IsDucking() ? VEC_DUCK_HULL_MIN : VEC_HULL_MIN );
		Vector vecMaxs = ( Client()->IsDucking() ? VEC_DUCK_HULL_MAX : VEC_HULL_MAX );

		float speed = atof( args[ 1 ] );
		float frametime = 1.f / atof( args[ 2 ] );

		g_pEngineFuncs->GetViewAngles( va );

		vecMove = static_cast<QAngle>( va ).GetForward() * speed * frametime;
		vecSimOrigin = g_pPlayerMove->origin + vecMove;

		//pPlayer->v.origin = vecSimOrigin;

		//DrawBox( vecSimOrigin, vecMins, vecMaxs, 1.f, 1.f, 0.f, 0.5f, 3.f, true );
		Render()->DrawBox( vecSimOrigin, vecMins, vecMaxs, { 1.f, 1.f, 0.f, 0.5f }, 10.f );
	}
}

CON_COMMAND( sc_st_follow_point, "Set local player view angles to a point and follow it" )
{
	if ( args.ArgC() != 5 )
	{
		Msg( "Usage: sc_st_follow_point <x> <y> <z> <lerp>\n" );
		s_bFollowPoint = false;

		return;
	}

	s_vecFollowPoint.x = atof( args[ 1 ] );
	s_vecFollowPoint.y = atof( args[ 2 ] );
	s_vecFollowPoint.z = atof( args[ 3 ] );
	s_flFollowPointLerp = atof( args[ 4 ] );

	s_bFollowPoint = true;
}

CON_COMMAND( sc_st_follow_point_stop, "Stop following point" )
{
	s_bFollowPoint = false;
}

CON_COMMAND( sc_st_setangles, "Set local player view angles" )
{
	if ( args.ArgC() != 4 )
	{
		Msg( "Usage: sc_st_setangles <pitch> <yaw> <frames>\n" );
		return;
	}

	int nFrames = atoi( args.Arg( 3 ) );

	if ( nFrames > 0 )
	{
		Vector va;
		g_pEngineFuncs->GetViewAngles( va );

		float flPitch = atof( args.Arg( 1 ) );
		float flYaw = atof( args.Arg( 2 ) );

		float flNormalizedPitch = Strafe::NormalizeDeg( static_cast<double>( flPitch ) - va[ PITCH ] );
		float flNormalizedYaw = Strafe::NormalizeDeg( static_cast<double>( flYaw ) - va[ YAW ] );

		s_vecSetAngles[ PITCH ] = flPitch;
		s_vecSetAngles[ YAW ] = flYaw;

		s_vecSetAnglesSpeed[ PITCH ] = std::abs( flNormalizedPitch ) / nFrames;
		s_vecSetAnglesSpeed[ YAW ] = std::abs( flNormalizedYaw ) / nFrames;

		s_bSetAngles = true;
	}
}

CON_COMMAND( sc_st_setangles_stop, "Stop setting angles" )
{
	s_bSetAngles = false;
}

CON_COMMAND( sc_st_setangles2, "Set local player view angles with given interpolation" )
{
	if ( args.ArgC() != 4 )
	{
		Msg( "Usage: sc_st_setangles <pitch> <yaw> <lerp>\n" );
		s_bSetAngles2 = false;
		return;
	}

	s_vecSetAngles2.x = atof( args[ 1 ] );
	s_vecSetAngles2.y = atof( args[ 2 ] );
	s_vecSetAngles2.z = 0.f;
	s_flSetAngles2Lerp = atof( args[ 3 ] );

	NormalizeAngles( s_vecSetAngles2 );

	s_bSetAngles2 = true;
}

CON_COMMAND( sc_st_setangles2_stop, "Stop setting angles" )
{
	s_bSetAngles2 = false;
}

//-----------------------------------------------------------------------------
// Incredible shit to find possible revivable spots
//-----------------------------------------------------------------------------

CON_COMMAND( sc_test_revive, "" )
{
	if ( SvenModAPI()->GetClientState() != CLS_ACTIVE )
		return;

	// settings
	float HULL_STEP = 4.f;
	bool DRAW_VALID_SPOTS_ONLY = false;

	if ( args.ArgC() > 1 )
		HULL_STEP = (float)atof( args[ 1 ] );
	
	if ( args.ArgC() > 2 )
		DRAW_VALID_SPOTS_ONLY = !!atoi( args[ 2 ] );

	Render()->DrawClear();

	if ( Host_IsServerActive() )
	{
		edict_t *pPlayer = g_pServerEngineFuncs->pfnPEntityOfEntIndex( g_pPlayerMove->player_index + 1 );

		if ( FNullEnt( pPlayer ) || !IsValidEntity( pPlayer ) )
			return;

		int flags;
		TraceResult tr;
		Vector vecOrigin, vecHullMins, vecHullMaxs;

		flags = pPlayer->v.flags;
		vecOrigin = pPlayer->v.origin;

		g_pServerEngineFuncs->pfnTraceHull( vecOrigin, vecOrigin, 0 /* dont_ignore_monsters */, 1 /* human_hull */, pPlayer, &tr );

		Msg( "sc_test_revive: Ducking = %d\n", ( flags & FL_DUCKING ) == FL_DUCKING );

		if ( flags & FL_DUCKING || tr.fStartSolid )
		{
			vecHullMins = VEC_DUCK_HULL_MIN;
			vecHullMaxs = VEC_DUCK_HULL_MAX;

			flags |= FL_DUCKING;
		}
		else
		{
			vecHullMins = VEC_HULL_MIN;
			vecHullMaxs = VEC_HULL_MAX;
		}

		Msg( "sc_test_revive: TraceHull, blocked = %d\n", tr.fStartSolid );

		Vector vecUpHead = vecOrigin + Vector( 0, 0, 32 ); // idk about name

		g_pServerEngineFuncs->pfnTraceLine( vecUpHead, vecOrigin, 1 /* ignore_monsters */, pPlayer, &tr );

		Msg( "sc_test_revive: TraceLine up, old = (%.3f, %.3f, %.3f), new = (%.3f, %.3f, %.3f)\n", VectorExpand( vecOrigin ), VectorExpand( tr.vecEndPos ) );

		vecOrigin = tr.vecEndPos;

		// FixPlayerCrouchStuck
		for ( int i = 0; i < 18; i++ )
		{
			Msg( "sc_test_revive: FixPlayerCrouchStuck, vecOrigin.z = %.3f\n", vecOrigin.z );

			g_pServerEngineFuncs->pfnTraceHull( vecOrigin, vecOrigin, 0 /* dont_ignore_monsters */, 3 /* head_hull */, pPlayer, &tr );

			if ( !tr.fStartSolid )
				break;

			vecOrigin.z += 1.f;
		}

		// FixPlayerStuck
		int maxsX, hulltype;
		float minX, maxX, minY, maxY, minZ, maxZ;
		Vector vecHull, vecTestOrigin;

		const int MAX_HULL_BOUND = 48;

		const Vector vecDebugBoxMins( -0.5, -0.5, -0.5 );
		const Vector vecDebugBoxMaxs( 0.5, 0.5, 0.5 );

		if ( vecHullMaxs.x > 0.f )
			maxsX = (int)vecHullMaxs.x;
		else
			maxsX = 8;

		vecHull.x = 16.f;
		vecHull.y = 16.f;

		if ( flags & FL_DUCKING )
		{
			vecHull.z = 36.f;
			hulltype = 3; // head_hull
		}
		else
		{
			vecHull.z = 72.f;
			hulltype = 1; // human_hull
		}

		if ( maxsX <= MAX_HULL_BOUND )
		{
			// Iterate from the largest hull, don't spend time and perfomance on small and medium ones
			for ( int i = MAX_HULL_BOUND; i <= MAX_HULL_BOUND; i += maxsX )
			//for ( int i = maxsX; i <= MAX_HULL_BOUND; i += maxsX )
			{
				Msg( "sc_test_revive: FixPlayerStuck, hull = %d\n", i );

				float hull = (float)i;

				minX = vecOrigin.x - hull;
				maxX = vecOrigin.x + hull;

				minY = vecOrigin.y - hull;
				maxY = vecOrigin.y + hull;

				minZ = vecOrigin.z - hull;
				maxZ = vecOrigin.z + hull;

				for ( float x = minX; x <= maxX; x += HULL_STEP )
				{
					for ( float y = minY; y <= maxY; y += HULL_STEP )
					{
						for ( float z = minZ; z <= maxZ; z += HULL_STEP )
						{
							vecTestOrigin.x = x;
							vecTestOrigin.y = y;
							vecTestOrigin.z = z;

							g_pServerEngineFuncs->pfnTraceHull( vecTestOrigin, vecHull, 0 /* dont_ignore_monsters */, hulltype, pPlayer, &tr );

							// Free space
							if ( !tr.fStartSolid )
							{
								Render()->DrawBox( vecTestOrigin, vecDebugBoxMins, vecDebugBoxMaxs, { 0, 255, 0, 127 }, 1e6 );
							}
							else if ( !DRAW_VALID_SPOTS_ONLY )
							{
								Render()->DrawBox( vecTestOrigin, vecDebugBoxMins, vecDebugBoxMaxs, { 255, 0, 0, 127 }, 1e6 );
							}
						}
					}
				}
			}
		}
	}
	else
	{
		// Client one is inconsistent but still it can give good results

		int flags;
		pmtrace_t tr;
		Vector vecOrigin, vecHullMins, vecHullMaxs;

		flags = g_pPlayerMove->flags;
		vecOrigin = g_pPlayerMove->origin;

		g_pEventAPI->EV_SetTraceHull( PM_HULL_PLAYER ); // human_hull
		g_pEventAPI->EV_PlayerTrace( vecOrigin, vecOrigin, PM_NORMAL, -1, &tr );

		Msg( "sc_test_revive: Ducking = %d\n", ( flags & FL_DUCKING ) == FL_DUCKING );

		if ( flags & FL_DUCKING || tr.startsolid )
		{
			vecHullMins = VEC_DUCK_HULL_MIN;
			vecHullMaxs = VEC_DUCK_HULL_MAX;

			flags |= FL_DUCKING;
		}
		else
		{
			vecHullMins = VEC_HULL_MIN;
			vecHullMaxs = VEC_HULL_MAX;
		}

		Msg( "sc_test_revive: TraceHull, blocked = %d\n", tr.startsolid );

		Vector vecUpHead = vecOrigin + Vector( 0, 0, 32 ); // idk about name

		// Inconsistent!!! No trace flag 'ignore_monsters' for the client
		g_pEventAPI->EV_SetTraceHull( PM_HULL_POINT );
		g_pEventAPI->EV_PlayerTrace( vecUpHead, vecOrigin, PM_NORMAL, -1, &tr );

		Msg( "sc_test_revive: TraceLine up, old = (%.3f, %.3f, %.3f), new = (%.3f, %.3f, %.3f)\n", VectorExpand( vecOrigin ), VectorExpand( tr.endpos ) );

		vecOrigin = tr.endpos;

		// FixPlayerCrouchStuck
		for ( int i = 0; i < 18; i++ )
		{
			Msg( "sc_test_revive: FixPlayerCrouchStuck, z = %.3f\n", vecOrigin.z );

			g_pEventAPI->EV_SetTraceHull( PM_HULL_DUCKED_PLAYER ); // head_hull but it's just ducked hull of player
			g_pEventAPI->EV_PlayerTrace( vecOrigin, vecOrigin, PM_NORMAL, -1, &tr );

			if ( !tr.startsolid )
				break;

			vecOrigin.z += 1.f;
		}

		// FixPlayerStuck
		int maxsX, hulltype;
		float minX, maxX, minY, maxY, minZ, maxZ;
		Vector vecHull, vecTestOrigin;

		const int MAX_HULL_BOUND = 48;

		const Vector vecDebugBoxMins( -0.5, -0.5, -0.5 );
		const Vector vecDebugBoxMaxs( 0.5, 0.5, 0.5 );

		if ( vecHullMaxs.x > 0.f )
			maxsX = (int)vecHullMaxs.x;
		else
			maxsX = 8;

		vecHull.x = 16.f;
		vecHull.y = 16.f;

		if ( flags & FL_DUCKING )
		{
			vecHull.z = 36.f;
			hulltype = PM_HULL_DUCKED_PLAYER; // head_hull but it's just ducked hull of player
		}
		else
		{
			vecHull.z = 72.f;
			hulltype = PM_HULL_PLAYER; // human_hull
		}

		if ( maxsX <= MAX_HULL_BOUND )
		{
			// Iterate from the largest hull, don't spend time and perfomance on small and medium ones
			for ( int i = MAX_HULL_BOUND; i <= MAX_HULL_BOUND; i += maxsX )
			//for ( int i = maxsX; i <= MAX_HULL_BOUND; i += maxsX )
			{
				Msg( "sc_test_revive: FixPlayerStuck, hull = %d\n", i );

				float hull = (float)i;

				minX = vecOrigin.x - hull;
				maxX = vecOrigin.x + hull;

				minY = vecOrigin.y - hull;
				maxY = vecOrigin.y + hull;

				minZ = vecOrigin.z - hull;
				maxZ = vecOrigin.z + hull;

				for ( float x = minX; x <= maxX; x += HULL_STEP )
				{
					for ( float y = minY; y <= maxY; y += HULL_STEP )
					{
						for ( float z = minZ; z <= maxZ; z += HULL_STEP )
						{
							vecTestOrigin.x = x;
							vecTestOrigin.y = y;
							vecTestOrigin.z = z;

							g_pEventAPI->EV_SetTraceHull( hulltype );
							g_pEventAPI->EV_PlayerTrace( vecTestOrigin, vecHull, PM_NORMAL, -1, &tr );

							// Free space
							if ( !tr.startsolid )
							{
								Render()->DrawBox( vecTestOrigin, vecDebugBoxMins, vecDebugBoxMaxs, { 0, 255, 0, 127 }, 1e6 );
							}
							else if ( !DRAW_VALID_SPOTS_ONLY )
							{
								Render()->DrawBox( vecTestOrigin, vecDebugBoxMins, vecDebugBoxMaxs, { 255, 0, 0, 127 }, 1e6 );
							}
						}
					}
				}
			}
		}
	}

	// Server-side playground
/*
	// FixPlayerStuck
	int maxsX, hulltype;
	float minX, maxX, minY, maxY, minZ, maxZ;
	Vector vecHull, vecTestOrigin;

	const int MAX_HULL_BOUND = 48;
	const float HULL_STEP = 2.f;

	if ( vecHullMaxs.x > 0.f )
		maxsX = (int)vecHullMaxs.x;
	else
		maxsX = 8;

	vecHull.x = 16.f;
	vecHull.y = 16.f;

	if ( flags & FL_DUCKING )
	{
		vecHull.z = 36.f;

	#if REVTEST_CLIENT
		hulltype = PM_HULL_DUCKED_PLAYER; // head_hull but it's just ducked hull of player
	#else
		hulltype = 3; // head_hull
	#endif
	}
	else
	{
		vecHull.z = 72.f;

	#if REVTEST_CLIENT
		hulltype = PM_HULL_PLAYER;
	#else
		hulltype = 1; // human_hull
	#endif
	}

	if ( maxsX <= MAX_HULL_BOUND )
	{
		int c = 0;

		for ( int i = MAX_HULL_BOUND; i <= MAX_HULL_BOUND; i += maxsX )
		//for ( int i = maxsX; i <= MAX_HULL_BOUND; i += maxsX )
		{
			Msg( "sc_test_revive: FixPlayerStuck: hulldelta = %d\n", i );

			float hull = (float)i;

			//Color clr;

			//switch ( c )
			//{
			//case 0:
			//	clr.r = 255;
			//	clr.g = 0;
			//	clr.b = 0;
			//	clr.a = 127;
			//	break;

			//case 1:
			//	clr.r = 0;
			//	clr.g = 255;
			//	clr.b = 0;
			//	clr.a = 127;
			//	break;

			//case 2:
			//	clr.r = 0;
			//	clr.g = 0;
			//	clr.b = 255;
			//	clr.a = 127;
			//	break;
			//}

			//c++;

			//for ( float x = vecOrigin.x - hull; x <= vecOrigin.x + hull; x += HULL_STEP )
			//{
			//	for ( float y = vecOrigin.y - hull; y <= vecOrigin.y + hull; y += HULL_STEP )
			//	{
			//		for ( float z = vecOrigin.z - hull; z <= vecOrigin.z + hull; z += HULL_STEP )
			//		{
			//			vecTestOrigin.x = x;
			//			vecTestOrigin.y = y;
			//			vecTestOrigin.z = z;

			//		#if REVTEST_CLIENT
			//			g_pEventAPI->EV_SetTraceHull( hulltype );
			//			g_pEventAPI->EV_PlayerTrace( vecTestOrigin, vecHull, PM_NORMAL, -1, &tr );
			//		#else
			//			g_pServerEngineFuncs->pfnTraceHull( vecTestOrigin, vecHull, 0, hulltype, pPlayer, &tr );
			//		#endif

			//			// Free space
			//		#if REVTEST_CLIENT
			//			if ( !tr.startsolid )
			//		#else
			//			if ( !tr.fStartSolid )
			//		#endif
			//			{
			//				//vecOrigin = vecTestOrigin;
			//				//break;

			//				Render()->DrawBox( vecTestOrigin, Vector( -0.5f, -0.5f, -0.5f ), Vector( 0.5f, 0.5f, 0.5f ), clr, 1e6 );
			//			}
			//			else
			//			{
			//				//Render()->DrawBox( vecTestOrigin, Vector( -0.5f, -0.5f, -0.5f ), Vector( 0.5f, 0.5f, 0.5f ), { 255, 0, 0, 127 }, 1e6 );
			//			}
			//		}
			//	}
			//}

			for ( int j = 0; j < 64; j++ )
			{
				minX = vecOrigin.x - hull;
				maxX = vecOrigin.x + hull;

				minY = vecOrigin.y - hull;
				maxY = vecOrigin.y + hull;

				minZ = vecOrigin.z - hull;
				maxZ = vecOrigin.z + hull;

				vecTestOrigin.x = g_pServerEngineFuncs->pfnRandomFloat( minX, maxX );
				vecTestOrigin.y = g_pServerEngineFuncs->pfnRandomFloat( minY, maxY );
				vecTestOrigin.z = g_pServerEngineFuncs->pfnRandomFloat( minZ, maxZ );

				g_pServerEngineFuncs->pfnTraceHull( vecTestOrigin, vecHull, 0, hulltype, pPlayer, &tr );

				// Free space
				if ( !tr.fStartSolid )
				{
					//vecOrigin = vecTestOrigin;
					//break;

					Render()->DrawBox( vecTestOrigin, Vector( -0.25f, -0.25f, -0.25f ), Vector( 0.25f, 0.25f, 0.25f ), { 0, 255, 0, 127 }, 1e6 );
				}
				else
				{
					Render()->DrawBox( vecTestOrigin, Vector( -0.25f, -0.25f, -0.25f ), Vector( 0.25f, 0.25f, 0.25f ), { 255, 0, 0, 127 }, 1e6 );
				}
			}
		}
	}
*/
}

//-----------------------------------------------------------------------------
// Hooks
//-----------------------------------------------------------------------------

static bool inside_CFuncTankGun_Fire = false;
static bool inside_CBaseEntity_FireBullets = false;
static void *inside_CBaseEntity_FireBullets_thisptr = NULL;

DECLARE_CLASS_FUNC( void, HOOKED_CBaseEntity_FireBullets, void *thisptr, unsigned int cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFeq, int iDamage, entvars_t *pAttacker, int fDraw )
{
	inside_CBaseEntity_FireBullets = true;
	inside_CBaseEntity_FireBullets_thisptr = thisptr;

	if ( sc_st_disable_spread.GetBool() )
	{
		vecSpread.Zero();
	}

	ORIG_CBaseEntity_FireBullets( thisptr, cShots, vecSrc, vecDirShooting, vecSpread, flDistance, iBulletType, iTracerFeq, iDamage, pAttacker, fDraw );

	inside_CBaseEntity_FireBullets = false;
}

DECLARE_CLASS_FUNC( void, HOOKED_CFuncTankGun_Fire, void *thisptr, const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
	inside_CFuncTankGun_Fire = true;

	ORIG_CFuncTankGun_Fire( thisptr, barrelEnd, forward, pevAttacker );

	inside_CFuncTankGun_Fire = false;
}

DECLARE_FUNC( void, __cdecl, HOOKED_UTIL_GetCircularGaussianSpread, float *x, float *y )
{
	ORIG_UTIL_GetCircularGaussianSpread( x, y );

	if ( sc_st_disable_spread.GetBool() )
	{
		*x = *y = 0.f;
	}
	
	//if ( inside_CFuncTankGun_Fire )
	//{
	//	scriptref_t hCallbackFunction;

	//	if ( hCallbackFunction = g_ScriptVM.LookupFunction( "OnTankGunFireSpread" ) )
	//	{
	//		lua_State *pLuaState = g_ScriptVM.GetVM();

	//		lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, (int)hCallbackFunction);

	//		lua_pushnumber( pLuaState, (lua_Number)*x );
	//		lua_pushnumber( pLuaState, (lua_Number)*y );

	//		g_ScriptVM.ProtectedCall( pLuaState, 2, 2, 0 );

	//		if ( lua_isnumber( pLuaState, -2 ) && lua_isnumber( pLuaState, -1 ) )
	//		{
	//			*x = (float)lua_tonumber( pLuaState, -2 );
	//			*y = (float)lua_tonumber( pLuaState, -1 );
	//		}

	//		g_ScriptVM.ReleaseFunction( hCallbackFunction );
	//	}
	//}

	if ( inside_CBaseEntity_FireBullets )
	{
		scriptref_t hCallbackFunction;

		if ( hCallbackFunction = g_ScriptVM.LookupFunction( "OnFireBulletsSpread" ) )
		{
			lua_State *pLuaState = g_ScriptVM.GetVM();

			entvars_t *pev = *(entvars_t **)( (unsigned long *)inside_CBaseEntity_FireBullets_thisptr + 1 );
			edict_t *pEntity = g_pServerEngineFuncs->pfnFindEntityByVars( pev );

			lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, (int)hCallbackFunction );

			lua_pushedict( pLuaState, pEntity );
			lua_pushinteger( pLuaState, (lua_Integer)g_pServerEngineFuncs->pfnIndexOfEdict( pEntity ) );
			lua_pushnumber( pLuaState, (lua_Number)*x );
			lua_pushnumber( pLuaState, (lua_Number)*y );

			g_ScriptVM.ProtectedCall( pLuaState, 4, 2, 0 );

			if ( lua_isnumber( pLuaState, -2 ) && lua_isnumber( pLuaState, -1 ) )
			{
				*x = (float)lua_tonumber( pLuaState, -2 );
				*y = (float)lua_tonumber( pLuaState, -1 );
			}

			g_ScriptVM.ReleaseFunction( hCallbackFunction );
		}
	}
}

// Timescale
DECLARE_FUNC( qboolean, __cdecl, HOOKED_Host_FilterTime, float time )
{
	static double timeCounter = 0.0;
	static bool usePassedTime = false;

	float minFrametime = sc_st_min_frametime.GetFloat();

	if ( minFrametime == 0.f )
	{
		timeCounter = 0.0;
		usePassedTime = false;
		return ORIG_Host_FilterTime( time );
	}

	timeCounter += time;

	if ( timeCounter < minFrametime )
		return 0;

	if ( ORIG_Host_FilterTime( usePassedTime ? time : static_cast<float>( timeCounter ) ) )
	{
		usePassedTime = false;
		timeCounter = std::fmod( timeCounter, minFrametime );
		return 1;
	}
	else
	{
		usePassedTime = true;
		return 0;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Callbacks
//-----------------------------------------------------------------------------

void CSpeedrunTools::OnBeginLoading( void )
{
	is_hl_c17 = false;

	iNihilanthIndex = 0;
	pNihilanthVars = NULL;

	StopTimer();
}

void CSpeedrunTools::OnFirstClientdataReceived( client_data_t *pcldata, float flTime )
{
	if ( Host_IsServerActive() )
	{
		const char *pszMapname = gpGlobals->pStringBase + gpGlobals->mapname;

		iNihilanthIndex = 0;
		pNihilanthVars = NULL;

		if ( !stricmp( pszMapname, "hl_c17" ) )
		{
			is_hl_c17 = true;

			edict_t *pNihilanth = NULL;

			if ( ( pNihilanth = g_pServerEngineFuncs->pfnFindEntityByString( NULL, "targetname", "nihilanth" ) ) != NULL )
			{
				iNihilanthIndex = g_pServerEngineFuncs->pfnIndexOfEdict( pNihilanth );
				pNihilanthVars = &pNihilanth->v;
			}
		}
		else
		{
			is_hl_c17 = false;
		}

		if ( sc_st_map_start_position.GetString()[ 0 ] != '\0' )
		{
			float x, y;

			int nParamsRead = sscanf( sc_st_map_start_position.GetString(), "%f %f", &x, &y );

			if ( nParamsRead >= 2 )
			{
				//Vector vecSpawnPoint;
				//Vector vecOrigin = pcldata->origin;

				//vecSpawnPoint.x = x;
				//vecSpawnPoint.y = y;

				//vecOrigin.z = 0.f;

				//if ( (vecOrigin - vecSpawnPoint).LengthSqr() > M_SQR(16.f) )
				//{
				//	g_pEngineFuncs->ClientCmd("restart\nwait");
				//	return;
				//}

				Vector vecSpawnPoint;
				Vector vecSpawnMins, vecSpawnMaxs;
				Vector vecMins, vecMaxs;

				vecSpawnPoint.x = x;
				vecSpawnPoint.y = y;

				vecMins = pcldata->origin + VEC_HULL_MIN;
				vecMaxs = pcldata->origin + VEC_HULL_MAX;

				vecSpawnMins = vecSpawnPoint + VEC_HULL_MIN;
				vecSpawnMaxs = vecSpawnPoint + VEC_HULL_MAX;

				vecMins.z = vecMaxs.z = 0.f;
				vecSpawnMins.z = vecSpawnMaxs.z = 0.f;

				if ( !UTIL_IsAABBIntersectingAABB( vecSpawnMins, vecSpawnMaxs, vecMins, vecMaxs ) )
				{
					g_pEngineFuncs->ClientCmd( "restart\nwait" );
					return;
				}

				//const float radius_sqr = M_SQR(16.f) * sqrtf(2.f);
				//float dmin = 0.f;

				//for (int i = 0; i < 2; i++)
				//{
				//	if ( vecSpawnPoint[i] < vecMins[i] )
				//	{
				//		dmin += M_SQR(vecSpawnPoint[i] - vecMins[i]);
				//	}
				//	else if ( vecSpawnPoint[i] > vecMaxs[i] )
				//	{
				//		dmin += M_SQR(vecSpawnPoint[i] - vecMaxs[i]);
				//	}
				//}

				//if ( dmin > radius_sqr )
				//{
				//	g_pEngineFuncs->ClientCmd("restart\nwait");
				//	return;
				//}
			}
		}
	}

	StartTimer();
}

//-----------------------------------------------------------------------------
// GameFrame
//-----------------------------------------------------------------------------

void CSpeedrunTools::GameFrame( void )
{
	CheckPlayerHulls_Server();

	BroadcastTimescale();

	if ( is_hl_c17 && iNihilanthIndex != 0 )
	{
		edict_t *pNihilanth = g_pServerEngineFuncs->pfnPEntityOfEntIndex( iNihilanthIndex );

		if ( pNihilanth != NULL && &pNihilanth->v == pNihilanthVars )
		{
			float flHealth = pNihilanthVars->health;

			if ( flHealth <= 0.f )
			{
				StopTimer();

				iNihilanthIndex = 0;
				pNihilanthVars = NULL;
			}
		}
		else
		{
			iNihilanthIndex = 0;
			pNihilanthVars = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// CreateMove
//-----------------------------------------------------------------------------

void CSpeedrunTools::CreateMove( float frametime, struct usercmd_s *cmd, int active )
{
	if ( ( s_bSetAngles || s_bSetAngles2 || s_bFollowPoint ) && ( !g_InputManager.IsInAction() || g_InputManager.IsRecording() ) )
	{
		Vector va;

		auto ChangeAngleBySpeed = []( float &flAngle, float flTargetAngle, float flChangeSpeed ) -> bool
		{
		#ifdef min
		#undef min
		#endif

			double adjustedTarget = Strafe::NormalizeDeg( (double)flTargetAngle );
			double normalizedDiff = Strafe::NormalizeDeg( adjustedTarget - (double)flAngle );
			double additionAbs = std::min( static_cast<double>( flChangeSpeed ), std::abs( normalizedDiff ) );

			flAngle = static_cast<float>( (double)flAngle + std::copysign( additionAbs, normalizedDiff ) );

			if ( std::abs( normalizedDiff ) > flChangeSpeed )
			{
				return true;
			}

			flAngle = flTargetAngle;
			return false;
		};
		
		if ( s_bFollowPoint )
		{
			Vector vecAngles, vecDir;

			g_pEngineFuncs->GetViewAngles( va );

			vecDir = s_vecFollowPoint - ( Client()->GetOrigin() + Client()->GetViewOffset() );

			vecAngles.x = -atan2f( vecDir.z, vecDir.Length2D() ) * (float)( 180.0 / M_PI );
			vecAngles.y = atan2f( vecDir.y, vecDir.x ) * (float)( 180.0 / M_PI );
			vecAngles.z = 0.f;

			NormalizeAngles( vecAngles );

			float flNormalizedPitch = Strafe::NormalizeDeg( vecAngles[ PITCH ] - va[ PITCH ] );
			float flNormalizedYaw = Strafe::NormalizeDeg( vecAngles[ YAW ] - va[ YAW ] );

			float flSetPitchSpeed = std::abs( flNormalizedPitch ) * s_flFollowPointLerp;
			float flSetYawSpeed = std::abs( flNormalizedYaw ) * s_flFollowPointLerp;

			bool bPitchChanged = ChangeAngleBySpeed( va[ PITCH ], vecAngles[ PITCH ], flSetPitchSpeed );
			bool bYawChanged = ChangeAngleBySpeed( va[ YAW ], vecAngles[ YAW ], flSetYawSpeed );

			NormalizeAngles( va );

			if ( bPitchChanged || bYawChanged )
			{
				g_pEngineFuncs->SetViewAngles( va );
				VectorCopy( va, cmd->viewangles );
			}
		}
		else if ( s_bSetAngles )
		{
			g_pEngineFuncs->GetViewAngles( va );

			va.y = NormalizeAngle( va.y );

			bool bPitchChanged = ChangeAngleBySpeed( va[ PITCH ], s_vecSetAngles[ PITCH ], s_vecSetAnglesSpeed[ PITCH ] );
			bool bYawChanged = ChangeAngleBySpeed( va[ YAW ], s_vecSetAngles[ YAW ], s_vecSetAnglesSpeed[ YAW ] );

			//if ( bPitchChanged )
			//	cmd->viewangles[PITCH] = va[PITCH];

			//if ( bYawChanged )
			//	cmd->viewangles[YAW] = va[YAW];

			NormalizeAngles( va );

			if ( !bPitchChanged && !bYawChanged )
			{
				s_bSetAngles = false;
			}
			else
			{
				g_pEngineFuncs->SetViewAngles( va );
				VectorCopy( va, cmd->viewangles );
			}
		}
		else if ( s_bSetAngles2 )
		{
			g_pEngineFuncs->GetViewAngles( va );

			float flNormalizedPitch = Strafe::NormalizeDeg( s_vecSetAngles2[ PITCH ] - va[ PITCH ] );
			float flNormalizedYaw = Strafe::NormalizeDeg( s_vecSetAngles2[ YAW ] - va[ YAW ] );

			float flSetPitchSpeed = std::abs( flNormalizedPitch ) * s_flSetAngles2Lerp;
			float flSetYawSpeed = std::abs( flNormalizedYaw ) * s_flSetAngles2Lerp;

			bool bPitchChanged = ChangeAngleBySpeed( va[ PITCH ], s_vecSetAngles2[ PITCH ], flSetPitchSpeed );
			bool bYawChanged = ChangeAngleBySpeed( va[ YAW ], s_vecSetAngles2[ YAW ], flSetYawSpeed );

			NormalizeAngles( va );

			if ( !bPitchChanged && !bYawChanged )
			{
				s_bSetAngles = false;
			}
			else
			{
				g_pEngineFuncs->SetViewAngles( va );
				VectorCopy( va, cmd->viewangles );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// VidInit
//-----------------------------------------------------------------------------

void CSpeedrunTools::OnVideoInit( void )
{
	for ( int i = 0; i < MAXCLIENTS + 1; i++ )
	{
		m_vPlayersHulls[ i ].time = -1.f;
	}

	m_bLegitMode = false;

	m_flTimerTime = 0.f;
	m_flLastTimerUpdate = -1.f;

	m_flDisplayHullsNextSend = -1.f;

	m_bShowReviveInfo = false;
	m_bShowReviveBoostInfo = false;
}

//-----------------------------------------------------------------------------
// CalcRefDef
//-----------------------------------------------------------------------------

void CSpeedrunTools::V_CalcRefDef( void )
{
	DrawReviveInfo();
	DrawPlayerHulls();
	DrawReviveBoostInfo();
	DrawReviveUnstuckArea();
	DrawLandPoint();
}

//-----------------------------------------------------------------------------
// HUD Redraw
//-----------------------------------------------------------------------------

void CSpeedrunTools::OnHUDRedraw( float time )
{
	if ( Host_IsServerActive() )
	{
		float flSegmentTime;

		if ( m_bSegmentStarted )
			flSegmentTime = gpGlobals->time - m_flSegmentStart;
		else
			flSegmentTime = m_flSegmentTime;

		ShowTimer( flSegmentTime, true );

		g_pServerEngineFuncs->pfnMessageBegin( MSG_BROADCAST, SVC_SVENINT, NULL, NULL );
		g_pServerEngineFuncs->pfnWriteByte( SVENINT_COMM_TIMER );
		g_pServerEngineFuncs->pfnWriteCoord( flSegmentTime );
		g_pServerEngineFuncs->pfnMessageEnd();
	}
	else if ( m_flLastTimerUpdate > 0.f && *dbRealtime - m_flLastTimerUpdate <= 1.f )
	{
		ShowTimer( m_flTimerTime, false );
	}
}

//-----------------------------------------------------------------------------
// 2D Draw
//-----------------------------------------------------------------------------

void CSpeedrunTools::Draw( void )
{
	m_engineFont = VGUI2_GetEngineFont();

	int r = int( 255.f * g_Config.cvars.st_hud_color[ 0 ] );
	int g = int( 255.f * g_Config.cvars.st_hud_color[ 1 ] );
	int b = int( 255.f * g_Config.cvars.st_hud_color[ 2 ] );

	ShowViewangles( r, g, b );
	ShowPosition( r, g, b );
	ShowVelocity( r, g, b );
	ShowGaussBoostInfo( r, g, b );
	ShowSelfgaussInfo( r, g, b );
	ShowEntityInfo( r, g, b );
	ShowReviveInfo( r, g, b );
	ShowReviveBoostInfo( r, g, b );

	DrawPlayersHullsNickname_Server();
}

//-----------------------------------------------------------------------------
// Current segment time
//-----------------------------------------------------------------------------

float CSpeedrunTools::SegmentCurrentTime( void )
{
	if ( Host_IsServerActive() && m_bSegmentStarted )
	{
		return gpGlobals->time - m_flSegmentStart;
	}

	return 0.f;
}

//-----------------------------------------------------------------------------
// Legit mode
//-----------------------------------------------------------------------------

void CSpeedrunTools::SetLegitMode( bool state )
{
	m_bLegitMode = state;
}

bool CSpeedrunTools::IsLegitMode( void ) const
{
	return m_bLegitMode;
}

//-----------------------------------------------------------------------------
// Show timer
//-----------------------------------------------------------------------------

void CSpeedrunTools::ShowTimer( float flTime, bool bServer )
{
	if ( !bServer )
	{
		m_flTimerTime = flTime;
		m_flLastTimerUpdate = *dbRealtime;
	}

	if ( !g_Config.cvars.st_timer )
		return;

	int minutes = static_cast<int>( flTime ) / 60;
	int seconds = static_cast<int>( flTime ) % 60;
	int ms = static_cast<int>( ( flTime - floorf( flTime ) ) * 1000.f );

	int iSpriteWidth = g_Drawing.GetNumberSpriteWidth();
	int iSpriteHeight = g_Drawing.GetNumberSpriteHeight();

	int iThickness = int( (float)iSpriteWidth / 8.f );

	int x = int( g_ScreenInfo.width * g_Config.cvars.st_timer_width_frac );
	int y = int( g_ScreenInfo.height * g_Config.cvars.st_timer_height_frac );

	int r = int( 255.f * g_Config.cvars.st_timer_color[ 0 ] );
	int g = int( 255.f * g_Config.cvars.st_timer_color[ 1 ] );
	int b = int( 255.f * g_Config.cvars.st_timer_color[ 2 ] );
	int a = 232;

	x += g_Drawing.DrawDigit( minutes / 10, x, y, r, g, b, FONT_ALIGN_LEFT );
	x += g_Drawing.DrawDigit( minutes % 10, x, y, r, g, b, FONT_ALIGN_LEFT );

	g_Drawing.FillArea( x + ( iSpriteWidth / 2 ) - iThickness,
						y + ( iSpriteHeight / 6 ),
						iThickness * 2,
						iThickness * 2,
						r, g, b, a );

	g_Drawing.FillArea( x + ( iSpriteWidth / 2 ) - iThickness,
						y + iSpriteHeight - ( iSpriteHeight / 4 ),
						iThickness * 2,
						iThickness * 2,
						r, g, b, a );

	x += iSpriteWidth;

	x += g_Drawing.DrawDigit( seconds / 10, x, y, r, g, b, FONT_ALIGN_LEFT );
	x += g_Drawing.DrawDigit( seconds % 10, x, y, r, g, b, FONT_ALIGN_LEFT );

	g_Drawing.FillArea( x + ( iSpriteWidth / 2 ) - iThickness,
						y + iSpriteHeight - ( iSpriteHeight / 4 ),
						iThickness * 2,
						iThickness * 2,
						r, g, b, a );

	x += iSpriteWidth;

	x += g_Drawing.DrawDigit( ms / 100, x, y, r, g, b, FONT_ALIGN_LEFT );
	x += g_Drawing.DrawDigit( ( ms / 10 ) % 10, x, y, r, g, b, FONT_ALIGN_LEFT );
	g_Drawing.DrawDigit( ms % 10, x, y, r, g, b, FONT_ALIGN_LEFT );
}

//-----------------------------------------------------------------------------
// Start timer
//-----------------------------------------------------------------------------

void CSpeedrunTools::StartTimer( void )
{
	if ( Host_IsServerActive() && !g_bPlayingbackDemo )
	{
		m_bSegmentStarted = true;
		m_flSegmentStart = gpGlobals->time;
		m_flSegmentTime = 0.f;

		if ( g_Config.cvars.st_timer )
		{
			ConColorMsg( { 255, 165, 0, 255 }, "> Started segment (map: %s)\n", gpGlobals->pStringBase + gpGlobals->mapname );
		}
	}
	else
	{
		m_bSegmentStarted = false;
		m_flSegmentStart = 0.f;
		m_flSegmentTime = 0.f;
	}
}

//-----------------------------------------------------------------------------
// Stop timer
//-----------------------------------------------------------------------------

void CSpeedrunTools::StopTimer( void )
{
	if ( g_Config.cvars.st_timer && Host_IsServerActive() && !g_bPlayingbackDemo )
	{
		if ( m_bSegmentStarted )
		{
			char timer_buffer[ 128 ];

			float flSegmentTime = m_flSegmentTime = gpGlobals->time - m_flSegmentStart;
			const char *pszMapname = gpGlobals->pStringBase + gpGlobals->mapname;

			int minutes = static_cast<int>( flSegmentTime ) / 60;
			int seconds = static_cast<int>( flSegmentTime ) % 60;
			int ms = static_cast<int>( ( flSegmentTime - floorf( flSegmentTime ) ) * 1000.f );

			snprintf( timer_buffer, M_ARRAYSIZE( timer_buffer ), "%d%d:%d%d,%d%d%d",
					  minutes / 10, minutes % 10,
					  seconds / 10, seconds % 10,
					  ms / 100, ( ms / 10 ) % 10, ms % 10 );

			Utils()->PrintChatText( "Finished segment in %s (%.6f) (map: %s)\n", timer_buffer, flSegmentTime, pszMapname );

			ConColorMsg( { 255, 165, 0, 255 }, "> Finished segment in " );
			ConColorMsg( { 179, 255, 32, 255 }, timer_buffer );
			ConColorMsg( { 122, 200, 0, 255 }, " (%.6f) ", flSegmentTime );
			ConColorMsg( { 255, 165, 0, 255 }, "(map: %s)\n", pszMapname );

			g_DemoMessage.WriteSegmentInfo( flSegmentTime, timer_buffer, pszMapname );
		}
	}

	m_bSegmentStarted = false;
	m_flSegmentStart = 0.f;
}

//-----------------------------------------------------------------------------
// Check existing players to display
//-----------------------------------------------------------------------------

void CSpeedrunTools::CheckPlayerHulls_Server( void )
{
	float flTime = *dbRealtime;

	if ( g_Config.cvars.st_server_player_hulls )
	{
		static Signatures::BasePlayer::IsAlive CBasePlayer__IsAlive = NULL;
		static Signatures::BasePlayer::IsConnected CBasePlayer__IsConnected = NULL;

		edict_t *pEntity = NULL;
		edict_t *pPlayerEdict = NULL;

		CBasePlayer *pPlayer = NULL;
		CBaseDeadPlayer *pDeadPlayer = NULL;

		while ( !FNullEnt( pEntity = g_pServerEngineFuncs->pfnFindEntityByString( pEntity, "classname", "deadplayer" ) ) )
		{
			if ( pEntity->v.effects & EF_NODRAW )
				continue;

			pDeadPlayer = reinterpret_cast<CBaseDeadPlayer *>( pEntity->pvPrivateData );

			if ( pDeadPlayer == NULL )
				continue;

			pPlayerEdict = pDeadPlayer->m_pPlayer;

			if ( pPlayerEdict == NULL || pPlayerEdict->serialnumber != pDeadPlayer->m_iPlayerSerialNumber || pPlayerEdict->free || pPlayerEdict->pvPrivateData == NULL )
				continue;

			int client = g_pServerEngineFuncs->pfnIndexOfEdict( pPlayerEdict );

			if ( m_flDisplayHullsNextSend <= flTime )
				BroadcastPlayerHull_Server( client, 1, pEntity->v.origin, int( pEntity->v.mins.z ) == -18 );

			DrawPlayerHull_Comm( client, 1, pEntity->v.origin, int( pEntity->v.mins.z ) == -18 );
		}

		// Now check players themselves
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			pEntity = g_pServerEngineFuncs->pfnPEntityOfEntIndex( i );

			if ( !IsValidEntity( pEntity ) )
				continue;

			pPlayer = reinterpret_cast<CBasePlayer *>( pEntity->pvPrivateData );

			if ( CBasePlayer__IsAlive == NULL )
			{
				CBasePlayer__IsAlive = (Signatures::BasePlayer::IsAlive)MemoryUtils()->GetVirtualFunction( pPlayer, Offsets::BasePlayer::IsAlive );
				CBasePlayer__IsConnected = (Signatures::BasePlayer::IsConnected)MemoryUtils()->GetVirtualFunction( pPlayer, Offsets::BasePlayer::IsConnected );

				AssertFatalMsg( CBasePlayer__IsAlive && CBasePlayer__IsConnected, "CBasePlayer::IsAlive && CBasePlayer::IsConnected" );
			}

			if ( !CBasePlayer__IsConnected( pPlayer ) || !CBasePlayer__IsAlive( pPlayer ) )
				continue;

			if ( m_flDisplayHullsNextSend <= flTime )
				BroadcastPlayerHull_Server( i, 0, pEntity->v.origin, int( pEntity->v.mins.z ) == -18 );

			DrawPlayerHull_Comm( i, 0, pEntity->v.origin, int( pEntity->v.mins.z ) == -18 );
		}
	}

	if ( m_flDisplayHullsNextSend <= flTime )
		m_flDisplayHullsNextSend = flTime + 0.05f;
}

//-----------------------------------------------------------------------------
// Draws nicknames of players to display
//-----------------------------------------------------------------------------

void CSpeedrunTools::DrawPlayersHullsNickname_Server( void )
{
	if ( !g_Config.cvars.st_server_player_hulls || g_Config.cvars.st_player_hulls )
		return;

	for ( int i = 1; i <= g_pEngineFuncs->GetMaxClients(); i++ )
	{
		playerhull_display_info_t &display_info = m_vPlayersHulls[ i ];

		if ( /* display_info.dead && */ display_info.time >= *dbRealtime )
		{
			float vecScreen[ 2 ];

			if ( UTIL_WorldToScreen( display_info.origin, vecScreen ) )
			{
				player_info_t *pPlayerInfo = g_pEngineStudio->PlayerInfo( i - 1 );

				if ( pPlayerInfo != NULL )
				{
					g_Drawing.DrawStringF( g_hFontESP, vecScreen[ 0 ], vecScreen[ 1 ], 255, 255, 255, 255, FONT_ALIGN_CENTER, pPlayerInfo->name );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Send player hull to everyone
//-----------------------------------------------------------------------------

void CSpeedrunTools::BroadcastPlayerHull_Server( int client, int dead, const Vector &vecOrigin, bool bDuck )
{
	struct
	{
		unsigned char client : 6;
		unsigned char dead : 1;
		unsigned char duck : 1;
	} displayInfo;

	displayInfo.client = client;
	displayInfo.dead = dead;
	displayInfo.duck = bDuck;

	//g_pServerEngineFuncs->pfnMessageBegin( MSG_PVS, SVC_SVENINT, NULL, NULL );
	g_pServerEngineFuncs->pfnMessageBegin( MSG_BROADCAST, SVC_SVENINT, NULL, NULL );
	g_pServerEngineFuncs->pfnWriteByte( SVENINT_COMM_DISPLAY_PLAYER_HULL );
	g_pServerEngineFuncs->pfnWriteByte( *(unsigned char *)&displayInfo );
	g_pServerEngineFuncs->pfnWriteLong( FloatToLong32( vecOrigin.x ) );
	g_pServerEngineFuncs->pfnWriteLong( FloatToLong32( vecOrigin.y ) );
	g_pServerEngineFuncs->pfnWriteLong( FloatToLong32( vecOrigin.z ) );
	g_pServerEngineFuncs->pfnMessageEnd();
}

//-----------------------------------------------------------------------------
// Draw player's hull
//-----------------------------------------------------------------------------

void CSpeedrunTools::DrawPlayerHull_Comm( int client, int dead, const Vector &vecOrigin, bool bDuck )
{
	if ( !g_Config.cvars.st_server_player_hulls || ( !dead && client == UTIL_GetLocalPlayerIndex() ) )
		return;

	playerhull_display_info_t &display_info = m_vPlayersHulls[ client ];

	if ( bDuck )
	{
		display_info.mins = VEC_DUCK_HULL_MIN;
		display_info.maxs = VEC_DUCK_HULL_MAX;
	}
	else
	{
		display_info.mins = VEC_HULL_MIN;
		display_info.maxs = VEC_HULL_MAX;
	}

	display_info.dead = dead;
	display_info.origin = vecOrigin;
	display_info.time = *dbRealtime + 1.f;
}

//-----------------------------------------------------------------------------
// Draw player hulls
//-----------------------------------------------------------------------------

void CSpeedrunTools::DrawPlayerHulls( void )
{
	if ( g_Config.cvars.st_server_player_hulls )
	{
		int iLocalPlayer = UTIL_GetLocalPlayerIndex();

		for ( int i = 1; i <= g_pEngineFuncs->GetMaxClients(); i++ )
		{
			playerhull_display_info_t &display_info = m_vPlayersHulls[ i ];

			if ( display_info.time >= *dbRealtime )
			{
				if ( iLocalPlayer == i && !g_Config.cvars.st_player_hulls_show_local_player )
					continue;

				if ( display_info.dead )
				{
					if ( g_Config.cvars.st_player_hulls_dead_color[ 3 ] == 0.f )
						continue;

					DrawBox( display_info.origin,
							 display_info.mins,
							 display_info.maxs,
							 g_Config.cvars.st_player_hulls_dead_color[ 0 ],
							 g_Config.cvars.st_player_hulls_dead_color[ 1 ],
							 g_Config.cvars.st_player_hulls_dead_color[ 2 ],
							 g_Config.cvars.st_player_hulls_dead_color[ 3 ],
							 g_Config.cvars.st_player_hulls_wireframe_width,
							 g_Config.cvars.st_player_hulls_show_wireframe );
				}
				else
				{
					if ( g_Config.cvars.st_player_hulls_color[ 3 ] == 0.f )
						continue;

					DrawBox( display_info.origin,
							 display_info.mins,
							 display_info.maxs,
							 g_Config.cvars.st_player_hulls_color[ 0 ],
							 g_Config.cvars.st_player_hulls_color[ 1 ],
							 g_Config.cvars.st_player_hulls_color[ 2 ],
							 g_Config.cvars.st_player_hulls_color[ 3 ],
							 g_Config.cvars.st_player_hulls_wireframe_width,
							 g_Config.cvars.st_player_hulls_show_wireframe );
				}
			}
		}
	}
	else if ( g_Config.cvars.st_player_hulls )
	{
		int iLocalPlayer = UTIL_GetLocalPlayerIndex();

		CEntity *pEnts = g_EntityList.GetList();

		for ( register int i = 1; i <= MY_MAXENTS; i++ )
		{
			CEntity &ent = pEnts[ i ];

			if ( !ent.m_bValid )
				continue;

			if ( ent.m_classInfo.id != CLASS_DEAD_PLAYER && !ent.m_bPlayer )
				continue;

			if ( iLocalPlayer == i && ( !g_Config.cvars.st_player_hulls_show_local_player || UTIL_IsSpectating() ) )
				continue;

			if ( ent.m_bPlayer )
			{
				if ( g_Config.cvars.st_player_hulls_color[ 3 ] == 0.f )
					continue;

				DrawBox( iLocalPlayer == i ? ( g_bPlayingbackDemo ? refparams.simorg : g_pPlayerMove->origin ) : ent.m_pEntity->origin,
						 ent.m_vecMins,
						 ent.m_vecMaxs,
						 g_Config.cvars.st_player_hulls_color[ 0 ],
						 g_Config.cvars.st_player_hulls_color[ 1 ],
						 g_Config.cvars.st_player_hulls_color[ 2 ],
						 g_Config.cvars.st_player_hulls_color[ 3 ],
						 g_Config.cvars.st_player_hulls_wireframe_width,
						 g_Config.cvars.st_player_hulls_show_wireframe );
			}
			else
			{
				if ( g_Config.cvars.st_player_hulls_dead_color[ 3 ] == 0.f )
					continue;

				DrawBox( ent.m_pEntity->origin,
						 ent.m_vecMins,
						 ent.m_vecMaxs,
						 g_Config.cvars.st_player_hulls_dead_color[ 0 ],
						 g_Config.cvars.st_player_hulls_dead_color[ 1 ],
						 g_Config.cvars.st_player_hulls_dead_color[ 2 ],
						 g_Config.cvars.st_player_hulls_dead_color[ 3 ],
						 g_Config.cvars.st_player_hulls_wireframe_width,
						 g_Config.cvars.st_player_hulls_show_wireframe );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Draw revive boost info
//-----------------------------------------------------------------------------

void CSpeedrunTools::DrawReviveBoostInfo( void )
{
	if ( g_Config.cvars.st_show_revive_boost_info &&
		 !UTIL_IsDead() &&
		 ( Client()->GetCurrentWeaponID() == WEAPON_MEDKIT || g_Config.cvars.st_show_revive_boost_any_weapon ) )
	{
		const Vector vecReviveHullMins( -16, -16, 0 );
		const Vector vecReviveHullMaxs( 16, 16, 72 );

		Vector vecRevivableTargetCenter;
		float flMinIntersection, flMaxIntersection;

		bool bDucking = ( g_bPlayingbackDemo ? refparams.viewheight[ 2 ] == VEC_DUCK_VIEW.z : g_pPlayerMove->flags & FL_DUCKING );

		CEntity *pEnts = g_EntityList.GetList();

		Vector vecCenter = ( g_bPlayingbackDemo ? refparams.simorg : g_pPlayerMove->origin );

		Vector vecMins = vecCenter + ( bDucking ? VEC_DUCK_HULL_MIN : VEC_HULL_MIN );
		Vector vecMaxs = vecCenter + ( bDucking ? VEC_DUCK_HULL_MAX : VEC_HULL_MAX );

		cl_entity_t *pTarget = NULL;

		for ( register int i = 1; i <= MY_MAXENTS; i++ )
		{
			CEntity &ent = pEnts[ i ];

			if ( !ent.m_bValid )
				continue;

			// Ignore invisible corpses of players
			if ( ent.m_pEntity->curstate.effects & EF_NODRAW )
				continue;

			// The only revivable targets are players in DEAD state, grenades and corpses of players
			if ( ( ent.m_bPlayer && !ent.m_bAlive ) || ( ent.m_classInfo.id == CLASS_ITEM_GRENADE && ( ent.m_bNeutral || ent.m_bEnemy ) ) || ent.m_classInfo.id == CLASS_DEAD_PLAYER )
			{
				vecRevivableTargetCenter = ent.m_vecOrigin;

				Vector vecRevivableTargetMins = vecRevivableTargetCenter + vecReviveHullMins;
				Vector vecRevivableTargetMaxs = vecRevivableTargetCenter + vecReviveHullMaxs;

				if ( pTarget == NULL )
				{
					if ( UTIL_IsAABBIntersectingAABB( vecMins, vecMaxs, vecRevivableTargetMins, vecRevivableTargetMaxs ) )
					{
						Vector vecDir = ( vecCenter - vecRevivableTargetCenter ).Normalize();

						if ( UTIL_IsRayIntersectingAABB( vecMins, vecMaxs, vecRevivableTargetCenter, vecDir, &flMinIntersection, &flMaxIntersection ) )
						{
							float flDistance, flDistanceToEdge;

							// Condition ( flMinIntersection < 0.f ) is met if ray has started inside AABB
							flDistance = ( flMinIntersection < 0.f ? flMaxIntersection : flMinIntersection );

							UTIL_IsRayIntersectingAABB( vecRevivableTargetMins, vecRevivableTargetMaxs, vecRevivableTargetCenter, vecDir, NULL, &flMaxIntersection );

							// Since everytime ray starts inside AABB of revivable target, we need only max distance of intersection since ( flMinIntersection < 0.f ) always true
							flDistanceToEdge = flMaxIntersection;

							// Draw boost direction
							if ( g_Config.cvars.st_show_revive_boost_info_direction_type == 0 ) // box
							{
								float flBeamExtent = g_Config.cvars.st_show_revive_boost_info_direction_box_extent;
								float flBeamLength = g_Config.cvars.st_show_revive_boost_info_direction_length;

								Vector vecBeamMins( 0.f, -flBeamExtent / 2, -flBeamExtent / 2 );
								Vector vecBeamMaxs( flBeamLength, flBeamExtent / 2, flBeamExtent / 2 );

								Vector vecAngles;

								VectorAngles( vecDir, vecAngles );
								vecAngles.x *= -1.f; // omg it's inverted, need to fix SDK?

								DrawBoxAngles( vecRevivableTargetCenter,
											   vecBeamMins,
											   vecBeamMaxs,
											   vecAngles,
											   g_Config.cvars.st_show_revive_boost_info_direction_color[ 0 ],
											   g_Config.cvars.st_show_revive_boost_info_direction_color[ 1 ],
											   g_Config.cvars.st_show_revive_boost_info_direction_color[ 2 ],
											   g_Config.cvars.st_show_revive_boost_info_direction_color[ 3 ],
											   g_Config.cvars.st_show_revive_boost_info_direction_line_width,
											   g_Config.cvars.st_show_revive_boost_info_wireframe_direction_box );
							}
							else // line
							{
								Render()->DrawLine( vecRevivableTargetCenter,
													vecRevivableTargetCenter + vecDir * g_Config.cvars.st_show_revive_boost_info_direction_length,
													g_Config.cvars.st_show_revive_boost_info_direction_color[ 0 ],
													g_Config.cvars.st_show_revive_boost_info_direction_color[ 1 ],
													g_Config.cvars.st_show_revive_boost_info_direction_color[ 2 ],
													g_Config.cvars.st_show_revive_boost_info_direction_color[ 3 ],
													g_Config.cvars.st_show_revive_boost_info_direction_line_width );
							}

							// Predict boost
							if ( g_Config.cvars.st_show_revive_boost_predict_trajectory || g_Config.cvars.st_show_revive_boost_predict_collision )
							{
								DrawPredictedReviveBoost();
							}

							m_pReviveBoostTarget = pTarget = ent.m_pEntity;
							// Distance from center of AABB of revivable target minus distance of local player's AABB
							m_flReviveBoostDistance = ( flDistanceToEdge >= flDistance ? ( flDistanceToEdge - flDistance ) : ( flDistance - flDistanceToEdge ) );
							m_flReviveBoostAngle = vecDir.z;
						}
					}
				}

				DrawBox( vecRevivableTargetCenter,
						 vecReviveHullMins,
						 vecReviveHullMaxs,
						 g_Config.cvars.st_show_revive_boost_info_hull_color[ 0 ],
						 g_Config.cvars.st_show_revive_boost_info_hull_color[ 1 ],
						 g_Config.cvars.st_show_revive_boost_info_hull_color[ 2 ],
						 g_Config.cvars.st_show_revive_boost_info_hull_color[ 3 ],
						 g_Config.cvars.st_show_revive_boost_info_wireframe_hull_width,
						 g_Config.cvars.st_show_revive_boost_info_wireframe_hull );
			}
		}

		if ( pTarget == NULL )
		{
			m_pReviveBoostTarget = NULL;
			m_flReviveBoostDistance = -1.f;
		}

		m_bShowReviveBoostInfo = true;
	}
}

//-----------------------------------------------------------------------------
// Predict revive boost
//-----------------------------------------------------------------------------

void CSpeedrunTools::DrawPredictedReviveBoost( void )
{
	if ( !g_bPlayingbackDemo && g_pPlayerMove->movevars == NULL )
		return;

	const int oldhull = g_pPlayerMove->usehull;
	const float flFrametime = 1.f / fps_max->value;

	pmtrace_t tr;
	bool bDucking, bOnGround;
	Vector vecOrigin, vecVelocity;
	CDrawTrajectory *pTrajectoryRenderer = NULL;

	int it = 0;

	if ( g_bPlayingbackDemo )
	{
		vecVelocity = refparams.simvel;
		vecOrigin = refparams.simorg;

		bDucking = ( refparams.viewheight[ 2 ] == VEC_DUCK_VIEW.z );
		bOnGround = false;

		// Trace forward
		tr = g_pPlayerMove->PM_PlayerTrace( vecOrigin, vecOrigin + ( vecVelocity * flFrametime ), PM_NORMAL, -1 );

		// Did hit a wall or started in solid
		if ( tr.fraction != 1.f && !tr.allsolid && tr.plane.normal.z >= 0.7f )
		{
			bOnGround = true;
		}
		else
		{
			Vector point = vecOrigin;
			point.z -= 2.f;

			// Trace down
			tr = g_pPlayerMove->PM_PlayerTrace( vecOrigin, point, PM_NORMAL, -1 );

			if ( tr.plane.normal.z >= 0.7f )
			{
				bOnGround = true;
			}
		}
	}
	else
	{
		if ( Host_IsServerActive() )
		{
			edict_t *pPlayer = g_pServerEngineFuncs->pfnPEntityOfEntIndex( g_pPlayerMove->player_index + 1 );

			if ( pPlayer == NULL )
				return;

			vecVelocity = pPlayer->v.velocity;
		}
		else
		{
			vecVelocity = g_pPlayerMove->velocity;
		}

		vecOrigin = g_pPlayerMove->origin;

		bDucking = ( g_pPlayerMove->flags & FL_DUCKING );
		bOnGround = ( g_pPlayerMove->onground != -1 );
	}

	// Set trace hull
	g_pPlayerMove->usehull = bDucking ? PM_HULL_DUCKED_PLAYER : PM_HULL_PLAYER;

	if ( g_Config.cvars.st_show_revive_boost_predict_trajectory )
	{
		Color clr = {
			g_Config.cvars.st_show_revive_boost_predict_trajectory_color[ 0 ],
			g_Config.cvars.st_show_revive_boost_predict_trajectory_color[ 1 ],
			g_Config.cvars.st_show_revive_boost_predict_trajectory_color[ 2 ],
			g_Config.cvars.st_show_revive_boost_predict_trajectory_color[ 3 ]
		};

		pTrajectoryRenderer = new CDrawTrajectory( clr, clr );
	}

	// Loop
	do
	{
		// Apply gravity
		if ( g_bPlayingbackDemo )
			UTIL_AddCorrectGravity( vecVelocity,
									refparams_movevars.gravity,
									refparams_movevars.entgravity,
									flFrametime );
		else
			UTIL_AddCorrectGravity( vecVelocity, flFrametime );

		Vector vecMove = vecVelocity * flFrametime;

		// Trace forward
		tr = g_pPlayerMove->PM_PlayerTrace( vecOrigin, vecOrigin + vecMove, PM_NORMAL, -1 );

		if ( g_Config.cvars.st_show_revive_boost_predict_trajectory )
			pTrajectoryRenderer->AddLine( vecOrigin, tr.endpos );

		// Save trace pos
		vecOrigin = tr.endpos;

		// Did hit a wall or started in solid
		if ( ( tr.fraction != 1.f && !tr.allsolid ) || tr.startsolid )
		{
			if ( g_Config.cvars.st_show_revive_boost_predict_collision )
			{
				DrawBox( vecOrigin,
						 bDucking ? VEC_DUCK_HULL_MIN : VEC_HULL_MIN,
						 bDucking ? VEC_DUCK_HULL_MAX : VEC_HULL_MAX,
						 g_Config.cvars.st_show_revive_boost_predict_collision_color[ 0 ],
						 g_Config.cvars.st_show_revive_boost_predict_collision_color[ 1 ],
						 g_Config.cvars.st_show_revive_boost_predict_collision_color[ 2 ],
						 g_Config.cvars.st_show_revive_boost_predict_collision_color[ 3 ],
						 g_Config.cvars.st_show_revive_boost_predict_collision_width,
						 true );
			}

			break;
		}

		if ( g_bPlayingbackDemo )
			UTIL_FixupGravityVelocity( vecVelocity,
									   refparams_movevars.gravity,
									   refparams_movevars.entgravity,
									   flFrametime );
		else
			UTIL_FixupGravityVelocity( vecVelocity, flFrametime );

		it++;

	} while ( it < 3000 );

	if ( g_Config.cvars.st_show_revive_boost_predict_trajectory )
		Render()->AddDrawContext( pTrajectoryRenderer );

	g_pPlayerMove->usehull = oldhull;
}

//-----------------------------------------------------------------------------
// Draw revive info
//-----------------------------------------------------------------------------

void CSpeedrunTools::DrawReviveInfo( void )
{
	int iWeaponID = Client()->GetCurrentWeaponID();

	if ( g_Config.cvars.st_show_revive_info &&
		 !UTIL_IsDead() &&
		 ( iWeaponID == WEAPON_MEDKIT || ( g_Config.cvars.st_show_revive_info_any_weapon ) ) )
	{
		pmtrace_t tr;
		Vector vecForward;

		int ignore_ent = -1;

		g_pEngineFuncs->AngleVectors( g_bPlayingbackDemo ? refparams.viewangles : g_pPlayerMove->angles, vecForward, NULL, NULL );

		Vector vecSrc = ( g_bPlayingbackDemo ? *(Vector *)refparams.simorg + refparams.viewheight : g_pPlayerMove->origin + g_pPlayerMove->view_ofs );
		Vector vecEnd = vecSrc + vecForward * 16.f;

		// Trace line
		g_pEventAPI->EV_SetTraceHull( PM_HULL_POINT );
		g_pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_NORMAL, ignore_ent, &tr );

		if ( tr.fraction >= 1.0 )
		{
			// Trace hull
			g_pEventAPI->EV_SetTraceHull( PM_HULL_DUCKED_PLAYER ); // in server-side head_hull has the same size as hull of ducked player
			//g_pEventAPI->EV_SetTraceHull( 3 ); // PM_HULL_HEAD, need to fix SDK
			g_pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_NORMAL, ignore_ent, &tr );

			if ( tr.fraction < 1.0 )
			{
				// Calculate the point of intersection of the line (or hull) and the object we hit
				// This is and approximation of the "best" intersection
				int ent = g_pEventAPI->EV_IndexFromTrace( &tr );
				cl_entity_t *pEntity = g_pEngineFuncs->GetEntityByIndex( ent );

				// pEntity == NULL || pEntity->IsBSPModel();
				if ( ent == 0 || ( pEntity != NULL && ( pEntity->curstate.solid == SOLID_BSP || pEntity->curstate.movetype == MOVETYPE_PUSHSTEP ) ) )
					UTIL_FindHullIntersectionClient( vecSrc, tr, Vector( -16, -16, -18 ), Vector( 16, 16, 18 ), ignore_ent ); // Duck hull

				vecEnd = tr.endpos;	// This is the point on the actual surface (the hull could have hit space)
			}
		}

		// Find target to revive within radius
		float flDistanceToTarget;
		Vector vecOrigin, vecAbsMins, vecAbsMaxs;

		cl_entity_t *pTarget = NULL;

		const float flTime = (float)*dbRealtime;
		const float flReviveRadius = 64.f;
		const float flReviveRadiusSqr = flReviveRadius * flReviveRadius;

		CEntity *pEnts = g_EntityList.GetList();

		for ( register int i = 1; i <= MY_MAXENTS; i++ )
		{
			CEntity &ent = pEnts[ i ];

			if ( !ent.m_bValid )
				continue;

			if ( ent.m_pEntity->curstate.effects & EF_NODRAW )
				continue;

			// The only revivable targets are players in DEAD state, grenades and corpses of players
			if ( ( ent.m_bPlayer && !ent.m_bAlive ) || ( ent.m_classInfo.id == CLASS_ITEM_GRENADE && ( ent.m_bNeutral || ent.m_bEnemy ) ) || ent.m_classInfo.id == CLASS_DEAD_PLAYER )
			{
				if ( ent.m_classInfo.id != CLASS_ITEM_GRENADE )
				{
					vecAbsMins = ent.m_vecOrigin + ent.m_vecMins;
					vecAbsMaxs = ent.m_vecOrigin + ent.m_vecMaxs;
				}
				else
				{
					vecAbsMins = ent.m_pEntity->curstate.origin + Vector( -1, -1, -1 );
					vecAbsMaxs = ent.m_pEntity->curstate.origin + Vector( 1, 1, 1 );
				}

				if ( !UTIL_IsSphereIntersectingAABB( vecEnd, flReviveRadiusSqr, vecAbsMins, vecAbsMaxs, &flDistanceToTarget ) )
					continue;

				// We got a player / their dead body / grenade within radius
				pTarget = ent.m_pEntity;
				vecOrigin = ent.m_pEntity->curstate.origin;
				break;
			}
		}

		// Find targets from transmitted server hulls
		if ( pTarget == NULL && g_Config.cvars.st_server_player_hulls )
		{
			cl_entity_t *pEntity = NULL;

			for ( int i = 1; i <= g_pEngineFuncs->GetMaxClients(); i++ )
			{
				playerhull_display_info_t &display_info = m_vPlayersHulls[ i ];

				if ( display_info.time >= flTime && display_info.dead )
				{
					pEntity = g_pEngineFuncs->GetEntityByIndex( i );

					if ( pEntity == NULL )
						continue;

					vecAbsMins = display_info.origin + display_info.mins;
					vecAbsMaxs = display_info.origin + display_info.maxs;

					if ( !UTIL_IsSphereIntersectingAABB( vecEnd, flReviveRadiusSqr, vecAbsMins, vecAbsMaxs, &flDistanceToTarget ) )
						continue;

					pTarget = pEntity;
					vecOrigin = display_info.origin;
					break;
				}
			}
		}

		// Valid target
		if ( pTarget != NULL )
		{
			float r, g, b, a;

			r = g_Config.cvars.st_show_revive_info_no_ammo_color[ 0 ];
			g = g_Config.cvars.st_show_revive_info_no_ammo_color[ 1 ];
			b = g_Config.cvars.st_show_revive_info_no_ammo_color[ 2 ];
			a = g_Config.cvars.st_show_revive_info_no_ammo_color[ 3 ];

			if ( iWeaponID == WEAPON_MEDKIT )
			{
				// Enough ammo to revive
				if ( ClientWeapon()->PrimaryAmmo() >= 50 )
				{
					r = g_Config.cvars.st_show_revive_info_color[ 0 ];
					g = g_Config.cvars.st_show_revive_info_color[ 1 ];
					b = g_Config.cvars.st_show_revive_info_color[ 2 ];
					a = g_Config.cvars.st_show_revive_info_color[ 3 ];
				}
			}
			else
			{
				WEAPON *pWeapon = Inventory()->GetWeapon( WEAPON_MEDKIT );

				if ( pWeapon != NULL && Inventory()->GetPrimaryAmmoCount( pWeapon ) >= 50 )
				{
					r = g_Config.cvars.st_show_revive_info_color[ 0 ];
					g = g_Config.cvars.st_show_revive_info_color[ 1 ];
					b = g_Config.cvars.st_show_revive_info_color[ 2 ];
					a = g_Config.cvars.st_show_revive_info_color[ 3 ];
				}
			}

			CDrawBoxNoDepthBuffer *pDrawBoxTarget = new CDrawBoxNoDepthBuffer( vecOrigin,
																			   Vector( -2, -2, -2 ),
																			   Vector( 2, 2, 2 ),
																			   { r, g, b, a } );

			Render()->AddDrawContext( pDrawBoxTarget );

			m_pReviveTarget = pTarget;
			m_flReviveDistance = flReviveRadius - flDistanceToTarget; // invert
		}
		else
		{
			m_pReviveTarget = NULL;
			m_flReviveDistance = -1.f;
		}

		m_bShowReviveInfo = true;
	}

	// Server-side test playground

	//if ( Host_IsServerActive() )
	//{
	//	// typedef enum { ignore_monsters=1, dont_ignore_monsters=0, missile=2 } IGNORE_MONSTERS;
	//	// typedef enum { point_hull=0, human_hull=1, large_hull=2, head_hull=3 };

	//	TraceResult tr;
	//	edict_t *pPlayer;

	//	pPlayer = g_pServerEngineFuncs->pfnPEntityOfEntIndex( g_pPlayerMove->player_index + 1 );

	//	g_pServerEngineFuncs->pfnMakeVectors( g_pPlayerMove->angles );

	//	bool bValidSpot = false;

	//	Vector vecSrc = g_pPlayerMove->origin + g_pPlayerMove->view_ofs;
	//	Vector vecEnd = vecSrc + gpGlobals->v_forward * 16.f;

	//	g_pServerEngineFuncs->pfnTraceLine( vecSrc, vecEnd, 0, pPlayer, &tr );

	//	if ( tr.flFraction >= 1.0 )
	//	{
	//		g_pServerEngineFuncs->pfnTraceHull( vecSrc, vecEnd, 0, 3, pPlayer, &tr );
	//		if ( tr.flFraction < 1.0 )
	//		{
	//			// Calculate the point of intersection of the line (or hull) and the object we hit
	//			// This is and approximation of the "best" intersection
	//			edict_t *pHit = tr.pHit;
	//			if ( !pHit || ( !FNullEnt( pHit ) && IsValidEntity( pHit ) && ( pHit->v.solid == SOLID_BSP || pHit->v.movetype == MOVETYPE_PUSHSTEP ) ) )
	//				UTIL_FindHullIntersection( vecSrc, tr, Vector( -16, -16, -18 ), Vector( 16, 16, 18 ), pPlayer );
	//			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
	//		}
	//	}

	//	float r = 1.f;
	//	float g = 1.f;
	//	float b = 1.f;

	//	edict_t *pEntity = NULL;

	//	while ( !FNullEnt( pEntity = g_pServerEngineFuncs->pfnFindEntityInSphere( pEntity, vecEnd, 64.f ) ) )
	//	{
	//		int index = g_pServerEngineFuncs->pfnIndexOfEdict( pEntity );

	//		if ( index == 1 )
	//			continue;

	//		if ( ( index > 0 && index <= gpGlobals->maxClients ) || !strcmp( gpGlobals->pStringBase + pEntity->v.classname, "deadplayer" ) )
	//		{
	//			r = b = 0.f;

	//			break;
	//		}
	//	}

	//	Render()->DrawBox( vecEnd, Vector( -64, -64, -64 ), Vector( 64, 64, 64 ), r, g, b, 0.5f );
	//}
}

//-----------------------------------------------------------------------------
// Draw revive / unstuck area
//-----------------------------------------------------------------------------

void CSpeedrunTools::DrawReviveUnstuckArea( void )
{
	if ( g_Config.cvars.st_show_revive_area_info )
	{
		pmtrace_t tr;
		bool bDucking;
		Vector vecOrigin;

		int iLocalPlayer = UTIL_GetLocalPlayerIndex();

		CEntity *pEnts = g_EntityList.GetList();

		for ( register int i = 1; i <= MY_MAXENTS; i++ )
		{
			CEntity &ent = pEnts[ i ];

			if ( !ent.m_bValid )
				continue;

			if ( ent.m_classInfo.id != CLASS_DEAD_PLAYER && !ent.m_bPlayer )
				continue;

			if ( iLocalPlayer == i && ( !g_Config.cvars.st_show_revive_area_local_player || UTIL_IsSpectating() ) )
				continue;

			bDucking = false;
			vecOrigin = ( iLocalPlayer == i ) ? ( g_bPlayingbackDemo ? refparams.simorg : g_pPlayerMove->origin ) : ent.m_pEntity->origin;

			if ( ent.m_bPlayer )
			{
				g_pEventAPI->EV_SetTraceHull( PM_HULL_PLAYER );
				g_pEventAPI->EV_PlayerTrace( vecOrigin, vecOrigin, PM_NORMAL, -1, &tr );

				// Clipped with world
				if ( tr.startsolid )
				{
					bDucking = true;
				}
			}
			else if ( ent.m_pEntity->curstate.movetype == MOVETYPE_NONE ) // Sinking corpse
			{
				bDucking = true;
			}

			// Inconsistent!! but it's needed. For perfomance reasons I don't want to trace line / hull
			if ( bDucking )
			{
				// FixPlayerCrouchStuck, trace hull
				vecOrigin.z += 18.f;
			}
			else
			{
				// Trace line up to 32 units
				vecOrigin.z += 32.f;
			}

			// Draw small hull
			if ( g_Config.cvars.st_show_revive_area_draw_small_hull )
			{
				DrawBox( vecOrigin,
						 ( bDucking ? VEC_DUCK_HULL_MIN : VEC_HULL_MIN ) + Vector( -16.f, -16.f, -16.f ),
						 ( bDucking ? VEC_DUCK_HULL_MAX : VEC_HULL_MAX ) + Vector( 16.f, 16.f, 16.f ),
						 g_Config.cvars.st_show_revive_area_small_hull_color[ 0 ],
						 g_Config.cvars.st_show_revive_area_small_hull_color[ 1 ],
						 g_Config.cvars.st_show_revive_area_small_hull_color[ 2 ],
						 g_Config.cvars.st_show_revive_area_small_hull_color[ 3 ],
						 g_Config.cvars.st_show_revive_area_small_hull_width,
						 true );
			}

			// Draw medium hull
			if ( g_Config.cvars.st_show_revive_area_draw_medium_hull )
			{
				DrawBox( vecOrigin,
						 ( bDucking ? VEC_DUCK_HULL_MIN : VEC_HULL_MIN ) + Vector( -32.f, -32.f, -32.f ),
						 ( bDucking ? VEC_DUCK_HULL_MAX : VEC_HULL_MAX ) + Vector( 32.f, 32.f, 32.f ),
						 g_Config.cvars.st_show_revive_area_medium_hull_color[ 0 ],
						 g_Config.cvars.st_show_revive_area_medium_hull_color[ 1 ],
						 g_Config.cvars.st_show_revive_area_medium_hull_color[ 2 ],
						 g_Config.cvars.st_show_revive_area_medium_hull_color[ 3 ],
						 g_Config.cvars.st_show_revive_area_medium_hull_width,
						 true );
			}

			if ( g_Config.cvars.st_show_revive_area_draw_large_hull )
			{
				// Draw large hull
				DrawBox( vecOrigin,
						 ( bDucking ? VEC_DUCK_HULL_MIN : VEC_HULL_MIN ) + Vector( -48.f, -48.f, -48.f ),
						 ( bDucking ? VEC_DUCK_HULL_MAX : VEC_HULL_MAX ) + Vector( 48.f, 48.f, 48.f ),
						 g_Config.cvars.st_show_revive_area_large_hull_color[ 0 ],
						 g_Config.cvars.st_show_revive_area_large_hull_color[ 1 ],
						 g_Config.cvars.st_show_revive_area_large_hull_color[ 2 ],
						 g_Config.cvars.st_show_revive_area_large_hull_color[ 3 ],
						 g_Config.cvars.st_show_revive_area_large_hull_width,
						 true );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Draw predicted landing
//-----------------------------------------------------------------------------

void CSpeedrunTools::DrawLandPoint( void )
{
	if ( !g_Config.cvars.st_show_land_point )
		return;

	if ( UTIL_IsDead() )
		return;

	if ( !g_bPlayingbackDemo && g_pPlayerMove->movevars == NULL )
		return;

	const int oldhull = g_pPlayerMove->usehull;
	const float flFrametime = 1.f / fps_max->value;

	pmtrace_t tr;
	Vector vecOrigin, vecVelocity;
	bool bDucking, bOnGround;

	int it = 0;
	int landings = 0;

	if ( g_bPlayingbackDemo )
	{
		vecVelocity = refparams.simvel;
		vecOrigin = refparams.simorg;

		bDucking = ( refparams.viewheight[ 2 ] == VEC_DUCK_VIEW.z );
		bOnGround = false;

		// Trace forward
		tr = g_pPlayerMove->PM_PlayerTrace( vecOrigin, vecOrigin + ( vecVelocity * flFrametime ), PM_NORMAL, -1 );

		// Did hit a wall or started in solid
		if ( tr.fraction != 1.f && !tr.allsolid && tr.plane.normal.z >= 0.7f )
		{
			bOnGround = true;
		}
		else
		{
			Vector point = vecOrigin;
			point.z -= 2.f;

			// Trace down
			tr = g_pPlayerMove->PM_PlayerTrace( vecOrigin, point, PM_NORMAL, -1 );

			if ( tr.plane.normal.z >= 0.7f )
			{
				bOnGround = true;
			}
		}
	}
	else
	{
		vecVelocity = g_pPlayerMove->velocity;
		vecOrigin = g_pPlayerMove->origin;

		bDucking = ( g_pPlayerMove->flags & FL_DUCKING );
		bOnGround = ( g_pPlayerMove->onground != -1 );
	}

	// PM_Jump
	if ( bOnGround )
	{
		vecVelocity.z += sqrtf( 2.f * 800.f * 45.f );

		if ( g_bPlayingbackDemo )
			UTIL_FixupGravityVelocity( vecVelocity,
									   refparams_movevars.gravity,
									   refparams_movevars.entgravity,
									   flFrametime );
		else
			UTIL_FixupGravityVelocity( vecVelocity, flFrametime );
	}

	// Set trace hull
	g_pPlayerMove->usehull = bDucking ? PM_HULL_DUCKED_PLAYER : PM_HULL_PLAYER;

	// Loop
	do
	{
		// Apply gravity
		if ( g_bPlayingbackDemo )
			UTIL_AddCorrectGravity( vecVelocity,
									refparams_movevars.gravity,
									refparams_movevars.entgravity,
									flFrametime );
		else
			UTIL_AddCorrectGravity( vecVelocity, flFrametime );

		Vector vecMove = vecVelocity * flFrametime;

		// Trace forward
		tr = g_pPlayerMove->PM_PlayerTrace( vecOrigin, vecOrigin + vecMove, PM_NORMAL, -1 );

		// Save trace pos
		vecOrigin = tr.endpos;

		// Did hit a wall or started in solid
		if ( ( tr.fraction != 1.f && !tr.allsolid ) || tr.startsolid )
		{
			if ( g_Config.cvars.st_show_land_point_draw_exact_point )
			{
				float flHeightShift = ( bDucking ? VEC_DUCK_HULL_MIN.z : VEC_HULL_MIN.z );

				DrawBox( vecOrigin + ( Vector( 0.f, 0.f, flHeightShift ) ),
						 Vector( -2, -2, 0 ),
						 Vector( 2, 2, 4 ),
						 g_Config.cvars.st_show_land_point_draw_exact_point_color[ 0 ],
						 g_Config.cvars.st_show_land_point_draw_exact_point_color[ 1 ],
						 g_Config.cvars.st_show_land_point_draw_exact_point_color[ 2 ],
						 g_Config.cvars.st_show_land_point_draw_exact_point_color[ 3 ],
						 0.f,
						 false );
			}

			if ( g_Config.cvars.st_show_land_point_draw_hull )
			{
				DrawBox( vecOrigin,
						 bDucking ? VEC_DUCK_HULL_MIN : VEC_HULL_MIN,
						 bDucking ? VEC_DUCK_HULL_MAX : VEC_HULL_MAX,
						 g_Config.cvars.st_show_land_point_draw_hull_color[ 0 ],
						 g_Config.cvars.st_show_land_point_draw_hull_color[ 1 ],
						 g_Config.cvars.st_show_land_point_draw_hull_color[ 2 ],
						 g_Config.cvars.st_show_land_point_draw_hull_color[ 3 ],
						 g_Config.cvars.st_show_land_point_draw_hull_width,
						 g_Config.cvars.st_show_land_point_draw_hull_wireframe );
			}

			landings++;

			if ( landings >= g_Config.cvars.st_show_land_point_max_points || vecVelocity.Length2DSqr() == 0.f )
				break;

			bool bPredictedOnGround = false;
			Vector vecWallNormal = tr.plane.normal;

			// Did hit a wall or started in solid
			if ( tr.fraction != 1.f && !tr.allsolid && tr.plane.normal.z >= 0.7f )
			{
				bPredictedOnGround = true;
			}
			else
			{
				Vector point = vecOrigin;
				point.z -= 2.f;

				// Trace down
				tr = g_pPlayerMove->PM_PlayerTrace( vecOrigin, point, PM_NORMAL, -1 );

				if ( tr.plane.normal.z >= 0.7f )
				{
					bPredictedOnGround = true;
				}
			}

			if ( bPredictedOnGround )
			{
				// PM_Jump
				vecVelocity.z = sqrtf( 2.f * 800.f * 45.f );

				if ( g_bPlayingbackDemo )
					UTIL_FixupGravityVelocity( vecVelocity,
											   refparams_movevars.gravity,
											   refparams_movevars.entgravity,
											   flFrametime );
				else
					UTIL_FixupGravityVelocity( vecVelocity, flFrametime );

				continue;
			}
			else
			{
				UTIL_ClipVelocity( vecVelocity, vecWallNormal, vecVelocity, 1.f );
			}
		}

		if ( g_bPlayingbackDemo )
			UTIL_FixupGravityVelocity( vecVelocity,
									   refparams_movevars.gravity,
									   refparams_movevars.entgravity,
									   flFrametime );
		else
			UTIL_FixupGravityVelocity( vecVelocity, flFrametime );

		it++;

	} while ( it < 3000 );

	g_pPlayerMove->usehull = oldhull;
}

//-----------------------------------------------------------------------------
// Send timescale to everyone
//-----------------------------------------------------------------------------

void CSpeedrunTools::BroadcastTimescale( void )
{
	if ( Host_IsServerActive() && fps_max->value != 20.f )
	{
		g_pServerEngineFuncs->pfnMessageBegin( MSG_BROADCAST, SVC_SVENINT, NULL, NULL );
		g_pServerEngineFuncs->pfnWriteByte( SVENINT_COMM_TIMESCALE );
		g_pServerEngineFuncs->pfnWriteByte( s_bNotifyTimescaleChanged ? 1 : 0 );
		g_pServerEngineFuncs->pfnWriteLong( FloatToLong32( host_framerate->value ) );
		g_pServerEngineFuncs->pfnWriteLong( FloatToLong32( fps_max->value ) );
		g_pServerEngineFuncs->pfnWriteLong( FloatToLong32( sc_st_min_frametime.GetFloat() ) );
		g_pServerEngineFuncs->pfnMessageEnd();

		s_bNotifyTimescaleChanged = false;
	}
}

//-----------------------------------------------------------------------------
// Send timescale to a single player
//-----------------------------------------------------------------------------

void CSpeedrunTools::SendTimescale( edict_t *pPlayer )
{
	if ( Host_IsServerActive() )
	{
		g_pServerEngineFuncs->pfnMessageBegin( MSG_ONE_UNRELIABLE, SVC_SVENINT, NULL, pPlayer );
		g_pServerEngineFuncs->pfnWriteByte( SVENINT_COMM_TIMESCALE );
		g_pServerEngineFuncs->pfnWriteByte( 1 );
		g_pServerEngineFuncs->pfnWriteLong( FloatToLong32( host_framerate->value ) );
		g_pServerEngineFuncs->pfnWriteLong( FloatToLong32( fps_max->value ) );
		g_pServerEngineFuncs->pfnWriteLong( FloatToLong32( sc_st_min_frametime.GetFloat() ) );
		g_pServerEngineFuncs->pfnMessageEnd();

		s_bNotifyTimescaleChanged = false;
	}
}

//-----------------------------------------------------------------------------
// Set timescale
//-----------------------------------------------------------------------------

void CSpeedrunTools::SetTimescale( float timescale )
{
	if ( timescale == 1.f )
	{
		sc_st_min_frametime.SetValue( 0.f );
		CVar()->SetValue( host_framerate, 0.f );
		return;
	}
	else if ( timescale > 1.f )
	{
		Msg( "Timescale can't be bigger than 1\n" );
		return;
	}
	else if ( timescale <= 0.f )
	{
		Msg( "Timescale must be bigger than 0\n" );
		return;
	}

	float multiplier = 1.f / timescale;

	sc_st_min_frametime.SetValue( multiplier / fps_max->value );
	CVar()->SetValue( host_framerate, 1.f / fps_max->value );

	Utils()->PrintChatText( "<SvenInt> Timescale has been set to %.2f\n", timescale );
}

//-----------------------------------------------------------------------------
// Set timescale from SvenInt user message
//-----------------------------------------------------------------------------

void CSpeedrunTools::SetTimescale_Comm( bool notify, float framerate, float fpsmax, float min_frametime )
{
	s_bIgnoreCvarChange = true;

	CVar()->SetValue( host_framerate, framerate );
	CVar()->SetValue( fps_max, fpsmax );

	sc_st_min_frametime.SetValue( min_frametime );

	if ( notify )
	{
		if ( min_frametime != 0.f )
			Utils()->PrintChatText( "<SvenInt-Comm> Timescale has been changed to %.2f\n", 1.f / ( min_frametime * fpsmax ) );
		else
			Utils()->PrintChatText( "<SvenInt-Comm> Timescale has been changed to 1.0\n" );
	}

	s_bIgnoreCvarChange = false;
}

//-----------------------------------------------------------------------------
// Draw view angles
//-----------------------------------------------------------------------------

void CSpeedrunTools::ShowViewangles( int r, int g, int b )
{
	if ( g_Config.cvars.st_show_view_angles )
	{
		Vector va;
		int width, height;

		int x = int( g_ScreenInfo.width * g_Config.cvars.st_show_view_angles_width_frac );
		int y = int( g_ScreenInfo.height * g_Config.cvars.st_show_view_angles_height_frac );

		if ( g_bPlayingbackDemo )
			va = refparams.viewangles;
		else
			g_pEngineFuncs->GetViewAngles( va );

		NormalizeAngles( va );

		g_Drawing.DrawStringEx( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "View angles:" );

		y += height;

		g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Pitch: %.6f", va.x );

		y += height;

		g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Yaw: %.6f", va.y );
	}
}

//-----------------------------------------------------------------------------
// Draw position
//-----------------------------------------------------------------------------

void CSpeedrunTools::ShowPosition( int r, int g, int b )
{
	if ( g_Config.cvars.st_show_pos )
	{
		Vector origin;
		int width, height;

		int x = int( g_ScreenInfo.width * g_Config.cvars.st_show_pos_width_frac );
		int y = int( g_ScreenInfo.height * g_Config.cvars.st_show_pos_height_frac );

		if ( g_bPlayingbackDemo )
			origin = refparams.simorg;
		else
			origin = g_pPlayerMove->origin;

		if ( g_Config.cvars.st_show_pos_view_origin )
		{
			if ( g_bPlayingbackDemo )
				origin += refparams.viewheight;
			else
				origin += g_pPlayerMove->view_ofs;
		}

		g_Drawing.DrawStringEx( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Origin:" );

		y += height;

		g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "X: %.6f", origin.x );

		y += height;

		g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Y: %.6f", origin.y );

		y += height;

		g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Z: %.6f", origin.z );
	}
}

//-----------------------------------------------------------------------------
// Draw velocity
//-----------------------------------------------------------------------------

void CSpeedrunTools::ShowVelocity( int r, int g, int b )
{
	if ( g_Config.cvars.st_show_velocity )
	{
		Vector velocity;
		int width, height;

		int x = int( g_ScreenInfo.width * g_Config.cvars.st_show_velocity_width_frac );
		int y = int( g_ScreenInfo.height * g_Config.cvars.st_show_velocity_height_frac );

		if ( g_bPlayingbackDemo )
			velocity = refparams.simvel;
		else
			velocity = g_pPlayerMove->velocity;

		g_Drawing.DrawStringEx( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Velocity:" );

		y += height;

		g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "X: %.6f", velocity.x );

		y += height;

		g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Y: %.6f", velocity.y );

		y += height;

		g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Z: %.6f", velocity.z );

		y += height;

		g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "XY: %.6f", velocity.Length2D() );

		y += height;

		g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "XYZ: %.6f", velocity.Length() );
	}
}

//-----------------------------------------------------------------------------
// Draw gauss info
//-----------------------------------------------------------------------------

void CSpeedrunTools::ShowGaussBoostInfo( int r, int g, int b )
{
	static cvar_t *sk_plr_secondarygauss = NULL;
	const float flGaussFullChargeTime = 3.f;

	if ( g_Config.cvars.st_show_gauss_boost_info && Client()->GetCurrentWeaponID() == WEAPON_GAUSS )
	{
		if ( sk_plr_secondarygauss == NULL )
		{
			sk_plr_secondarygauss = CVar()->FindCvar( "sk_plr_secondarygauss" );

			if ( sk_plr_secondarygauss == NULL )
				return;
		}

		float flYaw;
		int width, height;

		int x = int( g_ScreenInfo.width * g_Config.cvars.st_show_gauss_boost_info_width_frac );
		int y = int( g_ScreenInfo.height * g_Config.cvars.st_show_gauss_boost_info_height_frac );

		Vector velocity = ( g_bPlayingbackDemo ? refparams.simvel : g_pPlayerMove->velocity );

		g_Drawing.DrawStringEx( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Gauss boost info:" );

		y += height;

		if ( velocity.Length2DSqr() > 0.f )
		{
			flYaw = atan2f( velocity.y, velocity.x ) * 180.f / static_cast<float>( M_PI );

			if ( flYaw > 180.f )
				flYaw -= 360.f;
			else if ( flYaw < -180.f )
				flYaw += 360.f;

			g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Boost with optimal yaw: %.6f", flYaw );
		}
		else
		{
			flYaw = ( g_bPlayingbackDemo ? refparams.viewangles[ 1 ] : g_pPlayerMove->angles.y );

			g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Boost with optimal yaw: %.6f", flYaw );
		}

		y += height;

		weapon_data_t *pWeaponData = ClientWeapon()->GetWeaponData();

		int m_fInAttack = int( pWeaponData->fuser4 );
		float m_flStartChargeTime = fabs( pWeaponData->fuser2 );

		if ( m_fInAttack > 0 )
		{
			float flDamage;
			float flBoost;
			float flAmmoConsumed;

			float flSecondaryGaussDamage = sk_plr_secondarygauss->value;

			if ( flSecondaryGaussDamage <= 0.f )
				flSecondaryGaussDamage = 190.f; // default sven cope value

			if ( m_fInAttack == 1 )
			{
				flDamage = flSecondaryGaussDamage * ( 0.5f / flGaussFullChargeTime );

				flAmmoConsumed = 1.f;
			}
			else
			{
				if ( m_flStartChargeTime > flGaussFullChargeTime )
				{
					flDamage = flSecondaryGaussDamage;
				}
				else
				{
					flDamage = flSecondaryGaussDamage * ( m_flStartChargeTime / flGaussFullChargeTime );
				}

				flAmmoConsumed = ( m_flStartChargeTime - 0.5f ) / 0.3f;
				flAmmoConsumed += 1.f;
			}

			flBoost = flDamage * 5;

			g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Optimal resulting speed [back boost]: %.6f", velocity.Length2D() + flBoost );

			y += height;

			g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Overdose: %.6f", 10.f - m_flStartChargeTime );

			y += height;

			g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Damage: %.6f", flDamage );

			y += height;

			g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Boost: %.6f", flBoost );

			y += height;

			g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Ammo consumed: %.2f", flAmmoConsumed );
		}
		else
		{
			g_Drawing.DrawStringEx( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Optimal resulting speed [back boost]: N/A" );

			y += height;

			g_Drawing.DrawStringEx( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Overdose: 0.000000" );

			y += height;

			g_Drawing.DrawStringEx( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Damage: 0.000000" );

			y += height;

			g_Drawing.DrawStringEx( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Boost: 0.000000" );

			y += height;

			g_Drawing.DrawStringEx( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Ammo consumed: 0.00" );
		}
	}
}

//-----------------------------------------------------------------------------
// Draw selfgauss info
//-----------------------------------------------------------------------------

void CSpeedrunTools::ShowSelfgaussInfo( int r, int g, int b )
{
	static const char *HITGROUP_STRING[] =
	{
			"Generic",
			"Head",
			"Chest",
			"Stomach",
			"Left Arm",
			"Right Arm",
			"Left Leg",
			"Right Leg"
	};

	if ( g_Config.cvars.st_show_selfgauss_info && Host_IsServerActive() && Client()->GetCurrentWeaponID() == WEAPON_GAUSS )
	{
		int width, height;

		int x = int( g_ScreenInfo.width * g_Config.cvars.st_show_selfgauss_width_frac );
		int y = int( g_ScreenInfo.height * g_Config.cvars.st_show_selfgauss_height_frac );

		TraceResult tr;
		Vector va, forward, right, up, start, end;

		int iHitGroup;
		float flThreshold;

		bool bSelfgaussable = false;

		start = g_pPlayerMove->origin + g_pPlayerMove->view_ofs;

		g_pEngineFuncs->GetViewAngles( va );
		g_pEngineFuncs->AngleVectors( va, forward, right, up );

		end = start + forward * 8192.f;

		edict_t *pPlayer = g_pServerEngineFuncs->pfnPEntityOfEntIndex( g_pPlayerMove->player_index + 1 );

		if ( pPlayer == NULL )
			return;

		g_pServerEngineFuncs->pfnTraceLine( start, end, 0, pPlayer, &tr );

		if ( tr.pHit != NULL && tr.pHit->pvPrivateData != NULL && tr.pHit->v.solid == SOLID_BSP && !tr.pHit->v.takedamage )
		{
			float theta = -DotProduct( forward, tr.vecPlaneNormal );

			if ( theta >= 0.5f )
			{
				TraceResult beamTr;

				g_pServerEngineFuncs->pfnTraceLine( tr.vecEndPos + forward * 8, end, 0, NULL, &beamTr );

				if ( !beamTr.fAllSolid )
				{
					Vector vecBeamEndPos = beamTr.vecEndPos;

					g_pServerEngineFuncs->pfnTraceLine( vecBeamEndPos, tr.vecEndPos, 0, NULL, &beamTr );
					g_pServerEngineFuncs->pfnTraceLine( start, end, 0, NULL, &tr );

					bSelfgaussable = true;

					flThreshold = ( beamTr.vecEndPos - tr.vecEndPos ).Length();
					iHitGroup = tr.iHitgroup;
				}
			}
		}

		if ( bSelfgaussable )
		{
			g_Drawing.DrawStringEx( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Selfgauss:" );

			y += height;

			g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Threshold: %.6f", flThreshold );

			y += height;

			if ( iHitGroup < 0 || iHitGroup >= M_ARRAYSIZE( HITGROUP_STRING ) )
			{
				iHitGroup = 0;
			}

			g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Hit Group: %s", HITGROUP_STRING[ iHitGroup ] );
		}
		else
		{
			g_Drawing.DrawStringEx( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Cannot selfgauss" );
		}
	}
}

//-----------------------------------------------------------------------------
// Draw entity info
//-----------------------------------------------------------------------------

void CSpeedrunTools::ShowEntityInfo( int r, int g, int b )
{
	if ( g_Config.cvars.st_show_entity_info )
	{
		int width, height;

		int x = int( g_ScreenInfo.width * g_Config.cvars.st_show_entity_info_width_frac );
		int y = int( g_ScreenInfo.height * g_Config.cvars.st_show_entity_info_height_frac );

		Vector va, forward, start, end;

		start = ( g_bPlayingbackDemo ? *(Vector *)refparams.simorg + refparams.viewheight : g_pPlayerMove->origin + g_pPlayerMove->view_ofs );

		g_pEngineFuncs->GetViewAngles( va );
		g_pEngineFuncs->AngleVectors( va, forward, NULL, NULL );

		end = start + forward * 8192.f;

		if ( Host_IsServerActive() )
		{
			TraceResult tr;
			edict_t *pPlayer = g_pServerEngineFuncs->pfnPEntityOfEntIndex( g_pPlayerMove->player_index + 1 );

			if ( pPlayer == NULL )
				return;

			g_pServerEngineFuncs->pfnTraceLine( start, end, 0, pPlayer, &tr );

			if ( tr.pHit != NULL )
			{
				edict_t *pEntity = tr.pHit;
				int ent = g_pServerEngineFuncs->pfnIndexOfEdict( pEntity );
				bool bPlayer = ( ent >= 1 && ent <= g_pEngineFuncs->GetMaxClients() );

				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Entity: %d", ent );
				y += height;

				if ( bPlayer )
				{
					player_info_t *pPlayerInfo = g_pEngineStudio->PlayerInfo( ent - 1 );

					if ( pPlayerInfo != NULL )
					{
						g_Drawing.DrawStringEx( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, pPlayerInfo->name );
						y += height;

						g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "HP: %.6f", pEntity->v.health );
						y += height;

						g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Armor: %.6f", pEntity->v.armorvalue );
						y += height;

						g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Model: %s", pPlayerInfo->model );
						y += height;

						g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Top Color: %d", pPlayerInfo->topcolor );
						y += height;

						g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Bottom Color: %d", pPlayerInfo->bottomcolor );
						y += height;

						g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Steam64ID: %llu", pPlayerInfo->m_nSteamID );
						y += height;

						g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Pitch: %.6f", pEntity->v.v_angle.x );
						y += height;

						g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Yaw: %.6f", pEntity->v.v_angle.y );
						y += height;
					}
				}
				else if ( pEntity->v.classname != 0 )
				{
					const char *pszClassname = gpGlobals->pStringBase + pEntity->v.classname;

					g_Drawing.DrawStringEx( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, pszClassname );
					y += height;

					if ( pEntity->v.target != 0 )
					{
						g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Target: %s", gpGlobals->pStringBase + pEntity->v.target );
						y += height;
					}

					if ( pEntity->v.targetname != 0 )
					{
						g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Name: %s", gpGlobals->pStringBase + pEntity->v.targetname );
						y += height;
					}

					const char *pszModelName = ( ent == 0 ? "N/A" : ( pEntity->v.model ? gpGlobals->pStringBase + pEntity->v.model : "N/A" ) );

					g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Model: %s", pszModelName );
					y += height;

					if ( strstr( pszClassname, "func_door" ) )
					{
						g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Usable: %s", pEntity->v.spawnflags & 256 ? "Yes" : "No" );
						y += height;

						g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Monsters: %s open", pEntity->v.spawnflags & 512 ? "Can't" : "Can" );
						y += height;
					}

					if ( strstr( pszClassname, "func_door" ) || !strncmp( pszClassname, "func_rotating", 13 ) || !strncmp( pszClassname, "func_train", 10 ) )
					{
						g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Damage: %.6f", pEntity->v.dmg );
						y += height;
					}

					g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "HP: %.6f", pEntity->v.health );
					y += height;

					g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Yaw: %.6f", pEntity->v.angles.y );
					y += height;
				}

				Vector origin;

				if ( pEntity->v.solid == SOLID_BSP || pEntity->v.movetype == MOVETYPE_PUSHSTEP )
					origin = pEntity->v.origin + ( ( pEntity->v.mins + pEntity->v.maxs ) / 2.f );
				else
					origin = pEntity->v.origin;

				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "X: %.6f", origin.x );
				y += height;

				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Y: %.6f", origin.y );
				y += height;

				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Z: %.6f", origin.z );
				y += height;

				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "X Vel: %.6f", pEntity->v.velocity.x );
				y += height;

				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Y Vel: %.6f", pEntity->v.velocity.y );
				y += height;

				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Z Vel: %.6f", pEntity->v.velocity.z );
			}
			else
			{
				g_Drawing.DrawStringEx( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Entity: N/A" );
			}
		}
		else
		{
			pmtrace_t tr;

			g_pEventAPI->EV_SetTraceHull( PM_HULL_POINT );
			g_pEventAPI->EV_PlayerTrace( start, end, PM_NORMAL, -1, &tr );

			int ent = g_pEventAPI->EV_IndexFromTrace( &tr );

			if ( tr.fraction != 1.f )
			{
				if ( g_Config.cvars.st_show_entity_info_check_players )
				{
					struct eligible_player_t
					{
						float dist_sqr;
						int index;
					};

					std::vector<eligible_player_t> eligible_players;

					Vector traceEnd = tr.endpos;
					CEntity *pEnts = g_EntityList.GetList();
					int iLocalPlayer = UTIL_GetLocalPlayerIndex();

					for ( int i = 1; i <= g_pEngineFuncs->GetMaxClients(); i++ )
					{
						if ( i == iLocalPlayer )
							continue;

						CEntity &ent = pEnts[ i ];

						if ( !ent.m_bValid || !ent.m_bVisible || !ent.m_bAlive )
							continue;

						Vector vecMins, vecMaxs;

						vecMins = vecMaxs = ent.m_vecOrigin;

						if ( ent.m_bDucked )
						{
							vecMins += VEC_DUCK_HULL_MIN;
							vecMaxs += VEC_DUCK_HULL_MAX;
						}
						else
						{
							vecMins += VEC_HULL_MIN;
							vecMaxs += VEC_HULL_MAX;
						}

						if ( UTIL_IsLineIntersectingAABB( start, traceEnd, vecMins, vecMaxs ) )
						{
							eligible_players.push_back( { ( start - ent.m_vecOrigin ).LengthSqr(), i } );
						}
					}

					if ( !eligible_players.empty() )
					{
						if ( eligible_players.size() > 1 )
						{
							std::sort( eligible_players.begin(), eligible_players.end(), []( const eligible_player_t &a, const eligible_player_t &b )
							{
								return a.dist_sqr < b.dist_sqr;
							} );
						}

						ent = eligible_players[ 0 ].index;

						eligible_players.clear();
					}
				}

				cl_entity_t *pEntity = g_pEngineFuncs->GetEntityByIndex( ent );

				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Entity: %d", ent );
				y += height;

				if ( ent == 0 || pEntity->player )
				{
					if ( ent != 0 )
					{
						player_info_t *pPlayerInfo = g_pEngineStudio->PlayerInfo( ent - 1 );

						if ( pPlayerInfo != NULL )
						{
							g_Drawing.DrawStringEx( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, pPlayerInfo->name );
							y += height;

							g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "HP: %.6f", PlayerUtils()->GetHealth( ent ) );
							y += height;

							g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Armor: %.6f", PlayerUtils()->GetArmor( ent ) );
							y += height;

							g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Model: %s", pPlayerInfo->model );
							y += height;

							g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Top Color: %d", pPlayerInfo->topcolor );
							y += height;

							g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Bottom Color: %d", pPlayerInfo->bottomcolor );
							y += height;

							g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Steam64ID: %llu", pPlayerInfo->m_nSteamID );
							y += height;
						}
					}
					else
					{
						g_Drawing.DrawStringEx( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "worldspawn" );
						y += height;
					}
				}

				if ( !pEntity->player )
				{
					const char *pszModelName = ( ent == 0 ? "N/A" : ( pEntity->model ? pEntity->model->name : "N/A" ) );

					g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Model: %s", pszModelName );
					y += height;
				}
				else
				{
					g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Pitch: %.6f", pEntity->curstate.angles.x * ( 89.0f / 9.8876953125f ) );
					y += height;
				}

				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Yaw: %.6f", pEntity->curstate.angles.y );
				y += height;

				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "X: %.6f", pEntity->curstate.origin.x );
				y += height;

				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Y: %.6f", pEntity->curstate.origin.y );
				y += height;

				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Z: %.6f", pEntity->curstate.origin.z );
				//y += height;

				//g_Drawing.DrawStringEx(m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "X Vel: 0.000000");
				//y += height;

				//g_Drawing.DrawStringEx(m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Y Vel: 0.000000");
				//y += height;

				//g_Drawing.DrawStringEx(m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Z Vel: 0.000000");
			}
			else
			{
				g_Drawing.DrawStringEx( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Entity: N/A" );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Draw revive info
//-----------------------------------------------------------------------------

void CSpeedrunTools::ShowReviveInfo( int r, int g, int b )
{
	if ( m_bShowReviveInfo )
	{
		int width, height;

		int x = int( g_ScreenInfo.width * g_Config.cvars.st_show_revive_info_width_frac );
		int y = int( g_ScreenInfo.height * g_Config.cvars.st_show_revive_info_height_frac );

		if ( m_pReviveTarget != NULL )
		{
			if ( m_pReviveTarget->player )
			{
				player_info_t *pPlayerInfo = g_pEngineStudio->PlayerInfo( m_pReviveTarget->index - 1 );

				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Revive Target: %s (%d)",
										 pPlayerInfo ? pPlayerInfo->name : "N/A", m_pReviveTarget->index );
			}
			else
			{
				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Revive Target: %s (%d)",
										 m_pReviveTarget->model ? m_pReviveTarget->model->name : "DEADPLAYER", m_pReviveTarget->index );
			}
		}
		else
		{
			g_Drawing.DrawStringEx( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Revive Target: N/A" );
		}

		y += height;

		if ( m_flReviveDistance >= 0.f )
		{
			g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Distance: %.2f", m_flReviveDistance );
		}
		else
		{
			g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Distance: N/A" );
		}

		m_bShowReviveInfo = false;
	}
}

//-----------------------------------------------------------------------------
// Draw revive boost info
//-----------------------------------------------------------------------------

void CSpeedrunTools::ShowReviveBoostInfo( int r, int g, int b )
{
	if ( m_bShowReviveBoostInfo )
	{
		int width, height;

		int x = int( g_ScreenInfo.width * g_Config.cvars.st_show_revive_boost_info_width_frac );
		int y = int( g_ScreenInfo.height * g_Config.cvars.st_show_revive_boost_info_height_frac );

		if ( m_pReviveBoostTarget != NULL )
		{
			if ( m_pReviveBoostTarget->player )
			{
				player_info_t *pPlayerInfo = g_pEngineStudio->PlayerInfo( m_pReviveBoostTarget->index - 1 );

				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Revive Boost Target: %s (%d)",
										 pPlayerInfo ? pPlayerInfo->name : "N/A", m_pReviveBoostTarget->index );
			}
			else
			{
				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Revive Boost Target: %s (%d)",
										 m_pReviveBoostTarget->model ? m_pReviveBoostTarget->model->name : "DEADPLAYER", m_pReviveBoostTarget->index );
			}
		}
		else
		{
			g_Drawing.DrawStringEx( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Revive Boost Target: N/A" );
		}

		y += height;

		if ( m_flReviveBoostDistance >= 0.f )
		{
			float flVerticalEfficiency = fabs( 100.f * m_flReviveBoostAngle );
			float flHorizontalEfficiency = 100.f - flVerticalEfficiency;

			g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Distance: %.2f", m_flReviveBoostDistance );

			y += height;

			g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Vertical Boost Efficiency: %.2f %%", flVerticalEfficiency );

			y += height;

			g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Horizontal Boost Efficiency: %.2f %%", flHorizontalEfficiency );

			y += height;

			if ( Host_IsServerActive() )
			{
				edict_t *pPlayer = g_pServerEngineFuncs->pfnPEntityOfEntIndex( g_pPlayerMove->player_index + 1 );

				if ( pPlayer == NULL )
					return;

				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Vertical Speed: %.2f", fabs( pPlayer->v.velocity.z ) );

				y += height;

				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Horizontal Speed: %.2f", pPlayer->v.velocity.Length2D() );

				y += height;

				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Overall Speed: %.2f", pPlayer->v.velocity.Length() );
			}
			else
			{
				Vector vecVelocity = ( g_bPlayingbackDemo ? refparams.simvel : g_pPlayerMove->velocity );

				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Vertical Speed: %.2f", fabs( vecVelocity.z ) );

				y += height;

				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Horizontal Speed: %.2f", vecVelocity.Length2D() );

				y += height;

				g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Overall Speed: %.2f", vecVelocity.Length() );
			}
		}
		else
		{
			g_Drawing.DrawStringExF( m_engineFont, x, y, r, g, b, 255, width, height, FONT_ALIGN_LEFT, "Distance: N/A" );
		}

		m_bShowReviveBoostInfo = false;
	}
}

//-----------------------------------------------------------------------------
// Init
//-----------------------------------------------------------------------------

CSpeedrunTools::CSpeedrunTools()
{
	m_vPlayersHulls.reserve( MAXCLIENTS + 1 );

	for ( int i = 0; i < MAXCLIENTS + 1; i++ )
	{
		m_vPlayersHulls.push_back( { 0, Vector(), Vector(), Vector(), -1.f } );
	}

	m_bLegitMode = false;
	m_bSegmentStarted = false;

	m_flSegmentTime = 0.f;
	m_flSegmentStart = 0.f;

	m_flTimerTime = 0.f;
	m_flLastTimerUpdate = -1.f;

	m_flDisplayHullsNextSend = -1.f;

	m_bShowReviveInfo = false;
	m_pReviveTarget = NULL;
	m_flReviveDistance = 0.f;;

	m_bShowReviveBoostInfo = false;
	m_pReviveBoostTarget = NULL;
	m_flReviveBoostDistance = 0.f;
	m_flReviveBoostAngle = 0.f;

	m_pJumpOpCode = NULL;
	m_PatchedJumpOpCode = 0x9090;

	m_pfnHost_FilterTime = NULL;
	m_pfnCbuf_AddText = NULL;
	m_pfnServerCmd = NULL;

	m_engineFont = 0;
}

//-----------------------------------------------------------------------------
// Load feature
//-----------------------------------------------------------------------------

bool CSpeedrunTools::Load( void )
{
	ud_t inst;
	bool ScanOK = true;

	auto fpfnCBaseEntity_FireBullets = MemoryUtils()->FindPatternAsync( Sys_GetModuleHandle( "server.dll" ), CBaseEntity_FireBullets_sig );
	auto fpfnCFuncTankGun_Fire = MemoryUtils()->FindPatternAsync( Sys_GetModuleHandle( "server.dll" ), CFuncTankGun_Fire_sig );
	auto fpfnUTIL_GetCircularGaussianSpread = MemoryUtils()->FindPatternAsync( Sys_GetModuleHandle( "server.dll" ), UTIL_GetCircularGaussianSpread_sig );
	auto fpfnHost_FilterTime = MemoryUtils()->FindPatternAsync( SvenModAPI()->Modules()->Hardware, Host_FilterTime_sig );

	if ( !( m_pfnCBaseEntity_FireBullets = fpfnCBaseEntity_FireBullets.get() ) )
	{
		Warning( "Couldn't find function \"CBaseEntity::FireBullets\"\n" );
		ScanOK = false;
	}
	
	if ( !( m_pfnCFuncTankGun_Fire = fpfnCFuncTankGun_Fire.get() ) )
	{
		Warning( "Couldn't find function \"CFuncTankGun::Fire\"\n" );
		ScanOK = false;
	}
	
	if ( !( m_pfnUTIL_GetCircularGaussianSpread = fpfnUTIL_GetCircularGaussianSpread.get() ) )
	{
		Warning( "Couldn't find function \"UTIL_GetCircularGaussianSpread\"\n" );
		ScanOK = false;
	}

	if ( !( m_pfnHost_FilterTime = fpfnHost_FilterTime.get() ) )
	{
		Warning( "Couldn't find function \"Host_FilterTime\"\n" );
		ScanOK = false;
	}

	if ( !ScanOK )
		return false;

	m_pJumpOpCode = (unsigned short *)MemoryUtils()->FindPatternWithin( SvenModAPI()->Modules()->Hardware,
																		host_framerate_patch_sig,
																		m_pfnHost_FilterTime,
																		(unsigned char *)m_pfnHost_FilterTime + 128 );

	if ( !m_pJumpOpCode )
	{
		Warning( "Failed to locate jump condition of \"host_framerate\"\n" );
		return false;
	}

	unsigned char *pCallOpcode;

	int iDisassembledBytes = 0;
	void *pfnDrawConsoleString = g_pEngineFuncs->DrawConsoleString;

	if ( *(unsigned char *)pfnDrawConsoleString == 0xE9 ) // JMP
	{
		pfnDrawConsoleString = MemoryUtils()->CalcAbsoluteAddress( pfnDrawConsoleString );
	}
	else
	{
		Warning( "Failed to locate JMP op-code for function \"DrawConsoleString\"\n" );
		return false;
	}

	MemoryUtils()->InitDisasm( &inst, pfnDrawConsoleString, 32, 48 );

	pCallOpcode = (unsigned char *)pfnDrawConsoleString;

	while ( iDisassembledBytes = MemoryUtils()->Disassemble( &inst ) )
	{
		if ( inst.mnemonic == UD_Icall )
		{
			VGUI2_GetEngineFont = (VGUI2_GetEngineFontFn)MemoryUtils()->CalcAbsoluteAddress( pCallOpcode );
			break;
		}

		pCallOpcode += iDisassembledBytes;
	}

	if ( VGUI2_GetEngineFont == NULL )
	{
		Warning( "Failed to get function \"VGUI2_GetEngineFont\"\n" );
		return false;
	}

	m_PatchedJumpOpCode = *(unsigned short *)m_pJumpOpCode;

	return true;
}

//-----------------------------------------------------------------------------
// Post load feature
//-----------------------------------------------------------------------------

void CSpeedrunTools::PostLoad( void )
{
	if ( host_framerate == NULL )
		host_framerate = CVar()->FindCvar( "host_framerate" );

	if ( fps_max == NULL )
		fps_max = CVar()->FindCvar( "fps_max" );

	Hooks()->HookCvarChange( fps_max, CvarChangeHook_fps_max );

	m_hCBaseEntity_FireBullets = DetoursAPI()->DetourFunction( m_pfnCBaseEntity_FireBullets, HOOKED_CBaseEntity_FireBullets, GET_FUNC_PTR( ORIG_CBaseEntity_FireBullets ) );
	m_hCFuncTankGun_Fire = DetoursAPI()->DetourFunction( m_pfnCFuncTankGun_Fire, HOOKED_CFuncTankGun_Fire, GET_FUNC_PTR( ORIG_CFuncTankGun_Fire ) );
	m_hUTIL_GetCircularGaussianSpread = DetoursAPI()->DetourFunction( m_pfnUTIL_GetCircularGaussianSpread, HOOKED_UTIL_GetCircularGaussianSpread, GET_FUNC_PTR( ORIG_UTIL_GetCircularGaussianSpread ) );
	m_hHost_FilterTime = DetoursAPI()->DetourFunction( m_pfnHost_FilterTime, HOOKED_Host_FilterTime, GET_FUNC_PTR( ORIG_Host_FilterTime ) );

	*(unsigned short *)m_pJumpOpCode = 0x9090; // NOP NOP
}

//-----------------------------------------------------------------------------
// Unload feature
//-----------------------------------------------------------------------------

void CSpeedrunTools::Unload( void )
{
	*(unsigned short *)m_pJumpOpCode = m_PatchedJumpOpCode;

	Hooks()->UnhookCvarChange( fps_max, CvarChangeHook_fps_max );

	DetoursAPI()->RemoveDetour( m_hCBaseEntity_FireBullets );
	DetoursAPI()->RemoveDetour( m_hCFuncTankGun_Fire );
	DetoursAPI()->RemoveDetour( m_hUTIL_GetCircularGaussianSpread );
	DetoursAPI()->RemoveDetour( m_hHost_FilterTime );
}