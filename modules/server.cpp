#include "server.h"
#include "server_client_bridge.h"

#include "../patterns.h"
#include "../game/structs.h"
#include "../game/utils.h"
#include "../scripts/scripts.h"

#include <dbg.h>
#include <convar.h>
#include <ISvenModAPI.h>

//-----------------------------------------------------------------------------
// Hooks & function pointers
//-----------------------------------------------------------------------------

DECLARE_HOOK( void, __cdecl, Use, edict_t *, edict_t * );
DECLARE_HOOK( void, __cdecl, Touch, edict_t *, edict_t * );
DECLARE_HOOK( void, __cdecl, PlayerSpawns, edict_t *, edict_t * );
DECLARE_HOOK( bool, __cdecl, FixPlayerStuck, edict_t * );
DECLARE_HOOK( void, __cdecl, ClientKill, edict_t * );
DECLARE_HOOK( void, __cdecl, ClientPutInServer, edict_t * );
DECLARE_HOOK( void, __cdecl, ClientCommand, edict_t * );

DECLARE_CLASS_HOOK( void, CBasePlayer__SpecialSpawn, void * );
DECLARE_CLASS_HOOK( void, CBasePlayer__BeginRevive, void *, float );
DECLARE_CLASS_HOOK( void, CBasePlayer__EndRevive, void *, float );
DECLARE_CLASS_HOOK( entvars_t *, CopyPEntityVars, entvars_t *pev_dst, entvars_t *pev_src );

DECLARE_HOOK( void, __cdecl, FireTargets, const char *, void *, void *, int, float, float );

DECLARE_HOOK( void, __cdecl, RunPlayerMove, edict_t *, const float *, float, float, float, unsigned short, byte, byte );

FUNC_SIGNATURE( void *, __cdecl, GetSurvivalModeInstanceFn );
FUNC_SIGNATURE( void, __thiscall, CSurvivalMode__ToggleFn, void *thisptr );

GetSurvivalModeInstanceFn GetSurvivalModeInstance = NULL;
CSurvivalMode__ToggleFn CSurvivalMode__Toggle = NULL;

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

CServerModule g_ServerModule;

globalvars_t *gpGlobals = NULL;
globalvars_t **gpGlobals_ptr = NULL;

enginefuncs_t *g_pServerEngineFuncs = NULL;
DLL_FUNCTIONS *g_pServerFuncs = NULL;
NEW_DLL_FUNCTIONS *g_pNewServerFuncs = NULL;

Host_IsServerActiveFn Host_IsServerActive = NULL;

// stores dll funcs
static enginefuncs_t g_ServerEngineFuncs;
static DLL_FUNCTIONS g_ServerFuncs;
static NEW_DLL_FUNCTIONS g_NewServerFuncs;

static bool bRevivePreUnstuck = false;
static Vector vecRevivePreUnstuckOrigin;

//-----------------------------------------------------------------------------
// ConCommands
//-----------------------------------------------------------------------------

CON_COMMAND( setpos, "Set local player's position" )
{
	if ( args.ArgC() > 1 && Host_IsServerActive() )
	{
		edict_t *pPlayer = g_pServerEngineFuncs->pfnPEntityOfEntIndex( Client()->GetPlayerIndex() );

		if ( !FNullEnt( pPlayer ) && IsValidEntity( pPlayer ) )
		{
			Vector vecOrigin = pPlayer->v.origin;

			vecOrigin.x = atof( args[ 1 ] );

			if ( args.ArgC() > 2 )
			{
				vecOrigin.y = atof( args[ 2 ] );
			}

			if ( args.ArgC() > 3 )
			{
				vecOrigin.z = atof( args[ 3 ] );
			}

			pPlayer->v.origin = vecOrigin;
		}
	}
}

CON_COMMAND( setpos_exact, "Set local player's position" )
{
	if ( args.ArgC() > 1 && Host_IsServerActive() )
	{
		edict_t *pPlayer = g_pServerEngineFuncs->pfnPEntityOfEntIndex( Client()->GetPlayerIndex() );

		if ( !FNullEnt( pPlayer ) && IsValidEntity( pPlayer ) )
		{
			Vector vecOrigin = pPlayer->v.origin;

			vecOrigin.x = atof( args[ 1 ] );

			if ( args.ArgC() > 2 )
			{
				vecOrigin.y = atof( args[ 2 ] );
			}

			if ( args.ArgC() > 3 )
			{
				vecOrigin.z = atof( args[ 3 ] ) - pPlayer->v.view_ofs.z;
			}

			pPlayer->v.origin = vecOrigin;
		}
	}
}

CON_COMMAND( setvel, "Set local player's position" )
{
	if ( args.ArgC() > 1 && Host_IsServerActive() )
	{
		edict_t *pPlayer = g_pServerEngineFuncs->pfnPEntityOfEntIndex( Client()->GetPlayerIndex() );

		if ( !FNullEnt( pPlayer ) && IsValidEntity( pPlayer ) )
		{
			Vector vecVelocity = pPlayer->v.velocity;

			vecVelocity.x = atof( args[ 1 ] );

			if ( args.ArgC() > 2 )
			{
				vecVelocity.y = atof( args[ 2 ] );
			}

			if ( args.ArgC() > 3 )
			{
				vecVelocity.z = atof( args[ 3 ] );
			}

			pPlayer->v.velocity = vecVelocity;
		}
	}
}

//-----------------------------------------------------------------------------
// Hooks
//-----------------------------------------------------------------------------

DECLARE_FUNC( void, __cdecl, HOOKED_Use, edict_t *pUseEntity, edict_t *pOther )
{
	ORIG_Use( pUseEntity, pOther );

	g_ScriptCallbacks.OnEntityUse( pUseEntity, pOther );
}

DECLARE_FUNC( void, __cdecl, HOOKED_Touch, edict_t *pTouchEntity, edict_t *pOther )
{
	ORIG_Touch( pTouchEntity, pOther );

	g_ScriptCallbacks.OnEntityTouch( pTouchEntity, pOther );
}

DECLARE_FUNC( void, __cdecl, HOOKED_PlayerSpawns, edict_t *pSpawnSpot, edict_t *pPlayer )
{
	ORIG_PlayerSpawns( pSpawnSpot, pPlayer );

	g_ScriptCallbacks.OnPlayerSpawn( pSpawnSpot, pPlayer );
}

DECLARE_FUNC( bool, __cdecl, HOOKED_FixPlayerStuck, edict_t *pPlayer )
{
	bool bUnstuck = ORIG_FixPlayerStuck( pPlayer );

	if ( bUnstuck )
	{
		Vector unstuckBoundsMin = vecRevivePreUnstuckOrigin + Vector( -48.f, -48.f, -48.f );
		Vector unstuckBoundsMax = vecRevivePreUnstuckOrigin + Vector( 48.f, 48.f, 48.f );

		g_ScriptCallbacks.OnPlayerUnstuck( pPlayer );

		// Outside of the largest test hull !!
		if ( !UTIL_IsPointInsideAABB( pPlayer->v.origin, unstuckBoundsMin, unstuckBoundsMax ) )
		{
			g_pEngineFuncs->ClientCmd( "say \"FixPlayerStuck: NOT LEGIT UNSTUCK DETECTED.\"" );
			g_pEngineFuncs->ClientCmd( "say \"FixPlayerStuck: the unstuck position is outside the largest test hull.\"" );

			Warning( "FixPlayerStuck: pre-unstuck origin %.6f %.6f %.6f\n", VectorExpand( vecRevivePreUnstuckOrigin ) );
		}
	}

	return bUnstuck;
}

DECLARE_FUNC( void, __cdecl, HOOKED_ClientKill, edict_t *pPlayer )
{
	ORIG_ClientKill( pPlayer );

	g_ScriptCallbacks.OnClientKill( pPlayer );
}

DECLARE_FUNC( void, __cdecl, HOOKED_ClientPutInServer, edict_t *pPlayer )
{
	ORIG_ClientPutInServer( pPlayer );

	g_ServerClientBridge.OnClientPutInServer( pPlayer );
	g_ScriptCallbacks.OnClientPutInServer( pPlayer );
}

DECLARE_FUNC( void, __cdecl, HOOKED_ClientCommand, edict_t *pPlayer )
{
	if ( !pPlayer->pvPrivateData )
		return;

	if ( !strcmp( g_pServerEngineFuncs->pfnCmd_Argv( 0 ), "sc_sendsignal" ) )
	{
		if ( g_pServerEngineFuncs->pfnCmd_Argc() >= 2 )
		{
			g_ScriptCallbacks.OnClientSignal( pPlayer, atoi( g_pServerEngineFuncs->pfnCmd_Argv( 1 ) ) );
		}

		return;
	}

	ORIG_ClientCommand( pPlayer );
}

DECLARE_CLASS_FUNC( void, HOOKED_CBasePlayer__SpecialSpawn, void *thisptr )
{
	ORIG_CBasePlayer__SpecialSpawn( thisptr );

	// Skip vtable and then get player's entvars
	entvars_t *entvars = *(entvars_t **)( (unsigned long *)thisptr + 1 );
	edict_t *pPlayer = g_pServerEngineFuncs->pfnFindEntityByVars( entvars );

	if ( pPlayer != NULL )
	{
		g_ScriptCallbacks.OnSpecialSpawn( pPlayer );

		if ( bRevivePreUnstuck )
		{
			Vector unstuckBoundsMin = vecRevivePreUnstuckOrigin + Vector( -48.f, -48.f, -48.f );
			Vector unstuckBoundsMax = vecRevivePreUnstuckOrigin + Vector( 48.f, 48.f, 48.f );

			// Outside of the largest test hull !!
			if ( !UTIL_IsPointInsideAABB( pPlayer->v.origin, unstuckBoundsMin, unstuckBoundsMax ) )
			{
				g_pEngineFuncs->ClientCmd( "say \"CBasePlayer::SpecialSpawn -> FixPlayerStuck: NOT LEGIT REVIVE DETECTED.\"" );
				g_pEngineFuncs->ClientCmd( "say \"CBasePlayer::SpecialSpawn -> FixPlayerStuck: the revive position is outside the largest test hull.\"" );

				Warning( "FixPlayerStuck: pre-revive origin %.6f %.6f %.6f\n", VectorExpand( vecRevivePreUnstuckOrigin ) );
			}
		}
	}

	bRevivePreUnstuck = false;
}

DECLARE_CLASS_FUNC( void, HOOKED_CBasePlayer__BeginRevive, void *thisptr, float flNextThink )
{
	ORIG_CBasePlayer__BeginRevive( thisptr, flNextThink );

	entvars_t *entvars = *(entvars_t **)( (unsigned long *)thisptr + 1 );
	edict_t *pPlayer = g_pServerEngineFuncs->pfnFindEntityByVars( entvars );

	if ( pPlayer != NULL )
		g_ScriptCallbacks.OnBeginPlayerRevive( pPlayer );
}

DECLARE_CLASS_FUNC( void, HOOKED_CBasePlayer__EndRevive, void *thisptr, float flNextThink )
{
	ORIG_CBasePlayer__EndRevive( thisptr, flNextThink );

	entvars_t *entvars = *(entvars_t **)( (unsigned long *)thisptr + 1 );
	edict_t *pPlayer = g_pServerEngineFuncs->pfnFindEntityByVars( entvars );

	if ( pPlayer != NULL )
		g_ScriptCallbacks.OnEndPlayerRevive( pPlayer );
}

DECLARE_CLASS_FUNC( entvars_t *, HOOKED_CopyPEntityVars, entvars_t *pev_dst, entvars_t *pev_src )
{
	//Render()->DrawBox( pev_src->origin, Vector( -16, -16, -32 ), Vector( 16, 16, 32 ), { 255, 0, 0, 190 }, 30 );

	bRevivePreUnstuck = true;
	vecRevivePreUnstuckOrigin = pev_src->origin;

	return ORIG_CopyPEntityVars( pev_dst, pev_src );
}

DECLARE_FUNC( void, __cdecl, HOOKED_FireTargets, const char *pszTargetName, void *pActivator, void *pCaller, int useType, float flValue, float flDelay )
{
	ORIG_FireTargets( pszTargetName, pActivator, pCaller, useType, flValue, flDelay );

	g_ScriptCallbacks.OnFireTargets( pszTargetName, pActivator, pCaller, useType, flValue, flDelay );
}

//-----------------------------------------------------------------------------
// Server functions
//-----------------------------------------------------------------------------

bool IsSurvivalModeEnabled( void )
{
	CSurvivalMode *pSurvivalMode = reinterpret_cast<CSurvivalMode *>( GetSurvivalModeInstance() );

	if ( pSurvivalMode != NULL )
	{
		return pSurvivalMode->m_bEnabled;
	}

	return false;
}

bool EnableSurvivalMode( void )
{
	CSurvivalMode *pSurvivalMode = reinterpret_cast<CSurvivalMode *>( GetSurvivalModeInstance() );

	if ( pSurvivalMode != NULL )
	{
		bool bEnabled = pSurvivalMode->m_bEnabled;

		if ( !bEnabled )
		{
			pSurvivalMode->m_bEnabledNow = ( pSurvivalMode->m_bEnabled == false );

			CSurvivalMode__Toggle( pSurvivalMode );
			return true;
		}
	}

	return false;
}

bool DisableSurvivalMode( void )
{
	CSurvivalMode *pSurvivalMode = reinterpret_cast<CSurvivalMode *>( GetSurvivalModeInstance() );

	if ( pSurvivalMode != NULL )
	{
		bool bEnabled = pSurvivalMode->m_bEnabled;

		if ( bEnabled )
		{
			pSurvivalMode->m_bEnabledNow = ( pSurvivalMode->m_bEnabled == false );

			CSurvivalMode__Toggle( pSurvivalMode );
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// CServerModule
//-----------------------------------------------------------------------------

CServerModule::CServerModule()
{
	m_hServerDLL = NULL;

	m_pCBasePlayerVMT = NULL;
	m_pfnPlayerSpawns = NULL;
	m_pfnFixPlayerStuck = NULL;
	m_pfnCopyPEntityVars = NULL;

	m_hUse = DETOUR_INVALID_HANDLE;
	m_hTouch = DETOUR_INVALID_HANDLE;
	m_hPlayerSpawns = DETOUR_INVALID_HANDLE;
	m_hFixPlayerStuck = DETOUR_INVALID_HANDLE;
	m_hClientKill = DETOUR_INVALID_HANDLE;
	m_hClientPutInServer = DETOUR_INVALID_HANDLE;
	m_hCBasePlayer__SpecialSpawn = DETOUR_INVALID_HANDLE;
	m_hCopyPEntityVars = DETOUR_INVALID_HANDLE;
}

//-----------------------------------------------------------------------------
// Initialize server's library
//-----------------------------------------------------------------------------

bool CServerModule::Init( void )
{
	ud_t inst;
	bool ScanOK = true;

	m_hServerDLL = Sys_GetModuleHandle( "server.dll" );

	// Load server library
	if ( m_hServerDLL == NULL )
	{
		void *pfnSys_InitializeGameDLL = MemoryUtils()->FindPattern( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::Sys_InitializeGameDLL );

		if ( !pfnSys_InitializeGameDLL )
		{
			Warning( "Couldn't find function \"Sys_InitializeGameDLL\"\n" );
			return false;
		}

		( ( void ( * )( void ) )pfnSys_InitializeGameDLL )( );

		if ( ( m_hServerDLL = Sys_GetModuleHandle( "server.dll" ) ) == NULL )
		{
			Warning( "Failed to load server's binary\n" );
			return false;
		}

		Msg( "[Sven Internal] Preloaded server binary\n" );
	}

	// Get API functions
	int iNewDllFunctionsVersion = NEW_DLL_FUNCTIONS_VERSION;

	void *GiveFnptrsToDll = Sys_GetProcAddress( m_hServerDLL, "GiveFnptrsToDll" );
	APIFUNCTION GetEntityAPI = (APIFUNCTION)Sys_GetProcAddress( m_hServerDLL, "GetEntityAPI" );
	NEW_DLL_FUNCTIONS_FN GetNewDLLFunctions = (NEW_DLL_FUNCTIONS_FN)Sys_GetProcAddress( m_hServerDLL, "GetNewDLLFunctions" );

	if ( GiveFnptrsToDll == NULL )
	{
		Warning( "Failed to get function \"GiveFnptrsToDll\"\n" );
		return false;
	}

	if ( GetEntityAPI == NULL )
	{
		Warning( "Failed to get function \"GetEntityAPI\"\n" );
		return false;
	}

	if ( GetNewDLLFunctions == NULL )
	{
		Warning( "Failed to get function \"GetNewDLLFunctions\"\n" );
		return false;
	}

	if ( !GetEntityAPI( &g_ServerFuncs, INTERFACE_VERSION ) )
	{
		Warning( "Failed to get DLL functions\n" );
		return false;
	}

	if ( !GetNewDLLFunctions( &g_NewServerFuncs, &iNewDllFunctionsVersion ) )
	{
		Warning( "Failed to get new DLL functions\n" );
		return false;
	}

	enginefuncs_t *pServerEngineFuncs = NULL;

	MemoryUtils()->InitDisasm( &inst, GiveFnptrsToDll, 32, 32 );

	while ( MemoryUtils()->Disassemble( &inst ) )
	{
		if ( inst.mnemonic == UD_Imov && inst.operand[ 0 ].type == UD_OP_REG && inst.operand[ 0 ].base == UD_R_EDI && inst.operand[ 1 ].type == UD_OP_IMM )
		{
			memcpy( &g_ServerEngineFuncs, pServerEngineFuncs = reinterpret_cast<enginefuncs_t *>( inst.operand[ 1 ].lval.udword ), sizeof( enginefuncs_t ) );
			break;
		}
	}

	if ( pServerEngineFuncs == NULL )
	{
		Warning( "Failed to get server's engine functions\n" );
		return false;
	}

	g_pServerEngineFuncs = &g_ServerEngineFuncs;
	g_pServerFuncs = &g_ServerFuncs;
	g_pNewServerFuncs = &g_NewServerFuncs;

	// Async patterns scanning
	void *pfnCL_ClearState, *pgpGlobals, *pCBasePlayer_dtor_vmt;
	unsigned char *pfntoggle_survival_mode_Callback;

	auto fpfnCL_ClearState = MemoryUtils()->FindPatternAsync( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::CL_ClearState );
	auto fpgpGlobals = MemoryUtils()->FindPatternAsync( m_hServerDLL, Patterns::Server::gpGlobals );
	auto fpfntoggle_survival_mode_Callback = MemoryUtils()->FindPatternAsync( m_hServerDLL, Patterns::Server::toggle_survival_mode_Callback );
	auto fm_pfnCBasePlayer__SpecialSpawn = MemoryUtils()->FindPatternAsync( m_hServerDLL, Patterns::Server::CBasePlayer__SpecialSpawn );
	auto fm_pfnPlayerSpawns = MemoryUtils()->FindPatternAsync( m_hServerDLL, Patterns::Server::PlayerSpawns );
	auto fm_pfnFixPlayerStuck = MemoryUtils()->FindPatternAsync( m_hServerDLL, Patterns::Server::FixPlayerStuck );
	auto fpCBasePlayer_dtor_vmt = MemoryUtils()->FindPatternAsync( m_hServerDLL, Patterns::Server::CBasePlayer__vtable );
	auto fm_pfnCopyPEntityVars = MemoryUtils()->FindPatternAsync( m_hServerDLL, Patterns::Server::CopyPEntityVars );
	auto fm_pfnFireTargets = MemoryUtils()->FindPatternAsync( m_hServerDLL, Patterns::Server::FireTargets );

	if ( !( pfnCL_ClearState = fpfnCL_ClearState.get() ) )
	{
		Warning( "Failed to locate function \"Host_IsServerActive\"\n" );
		ScanOK = false;
	}

	if ( !( pgpGlobals = fpgpGlobals.get() ) )
	{
		Warning( "Couldn't locate \"gpGlobals\"\n" );
		ScanOK = false;
	}

	if ( !( pfntoggle_survival_mode_Callback = (unsigned char *)fpfntoggle_survival_mode_Callback.get() ) )
	{
		Warning( "Couldn't locate \"toggle_survival_mode_Callback\"\n" );
		ScanOK = false;
	}

	if ( !( m_pfnPlayerSpawns = fm_pfnPlayerSpawns.get() ) )
	{
		Warning( "Failed to locate function \"PlayerSpawns\"\n" );
		ScanOK = false;
	}
	
	if ( !( m_pfnFixPlayerStuck = fm_pfnFixPlayerStuck.get() ) )
	{
		Warning( "Failed to locate function \"FixPlayerStuck\"\n" );
		ScanOK = false;
	}

	if ( !( pCBasePlayer_dtor_vmt = fpCBasePlayer_dtor_vmt.get() ) )
	{
		Warning( "Failed to locate VMT of CBasePlayer class\n" );
		ScanOK = false;
	}
	
	if ( !( m_pfnCopyPEntityVars = fm_pfnCopyPEntityVars.get() ) )
	{
		Warning( "Failed to locate function call \"CopyPEntityVars\"\n" );
		ScanOK = false;
	}
	
	if ( !( m_pfnFireTargets = fm_pfnFireTargets.get() ) )
	{
		Warning( "Failed to locate function \"FireTargets\"\n" );
		ScanOK = false;
	}

	if ( !ScanOK )
		return false;

	// Get Host_IsServerActive
	MemoryUtils()->InitDisasm( &inst, pfnCL_ClearState, 32, 16 );

	if ( MemoryUtils()->Disassemble( &inst ) )
	{
		if ( inst.mnemonic == UD_Icall )
		{
			Host_IsServerActive = (Host_IsServerActiveFn)MemoryUtils()->CalcAbsoluteAddress( pfnCL_ClearState );
		}
	}
	else
	{
		Warning( "Couldn't locate function \"Host_IsServerActive\" #2\n" );
		return false;
	}

	if ( !Host_IsServerActive )
	{
		Warning( "Failed to get function \"Host_IsServerActive\"\n" );
		return false;
	}

	// Get gpGlobals
	MemoryUtils()->InitDisasm( &inst, pgpGlobals, 32, 16 );

	if ( MemoryUtils()->Disassemble( &inst ) )
	{
		if ( inst.mnemonic == UD_Imov && inst.operand[ 0 ].type == UD_OP_REG && inst.operand[ 0 ].base == UD_R_EAX && inst.operand[ 1 ].type == UD_OP_MEM )
		{
			gpGlobals_ptr = reinterpret_cast<globalvars_t **>( inst.operand[ 1 ].lval.udword );
		}
	}
	else
	{
		Warning( "Couldn't locate \"gpGlobals\" #2\n" );
		return false;
	}

	if ( !gpGlobals_ptr || !( gpGlobals = *gpGlobals_ptr ) )
	{
		Warning( "Failed to get \"gpGlobals\"\n" );
		return false;
	}

	// Survival mode functions
	int iDisassembledBytes = 0;
	MemoryUtils()->InitDisasm( &inst, pfntoggle_survival_mode_Callback, 32, 24 );

	while ( iDisassembledBytes = MemoryUtils()->Disassemble( &inst ) )
	{
		if ( inst.mnemonic == UD_Icall )
		{
			GetSurvivalModeInstance = (GetSurvivalModeInstanceFn)MemoryUtils()->CalcAbsoluteAddress( pfntoggle_survival_mode_Callback );
		}
		else if ( inst.mnemonic == UD_Ijmp )
		{
			CSurvivalMode__Toggle = (CSurvivalMode__ToggleFn)MemoryUtils()->CalcAbsoluteAddress( pfntoggle_survival_mode_Callback );
			break;
		}

		pfntoggle_survival_mode_Callback += iDisassembledBytes;
	}

	if ( !GetSurvivalModeInstance )
	{
		Warning( "Failed to get \"GetSurvivalModeInstance\"\n" );
		return false;
	}

	if ( !CSurvivalMode__Toggle )
	{
		Warning( "Failed to get \"CSurvivalMode::Toggle\"\n" );
		return false;
	}

	// Get VMT of CBasePlayer
	m_pCBasePlayerVMT = *(void **)( (unsigned char *)pCBasePlayer_dtor_vmt + 2 );

	// Tertiary attack glitch
	extern void InitTertiaryAttackGlitch_Server( HMODULE hServerDLL );
	InitTertiaryAttackGlitch_Server( m_hServerDLL );

	return true;
}

//-----------------------------------------------------------------------------
// Post initialize server's library
//-----------------------------------------------------------------------------

void CServerModule::PostInit( void )
{
	void *dummyBasePlayer = m_pCBasePlayerVMT;

	m_hUse = DetoursAPI()->DetourFunction( g_pServerFuncs->pfnUse, HOOKED_Use, GET_FUNC_PTR( ORIG_Use ) );
	m_hTouch = DetoursAPI()->DetourFunction( g_pServerFuncs->pfnTouch, HOOKED_Touch, GET_FUNC_PTR( ORIG_Touch ) );
	m_hClientKill = DetoursAPI()->DetourFunction( g_pServerFuncs->pfnClientKill, HOOKED_ClientKill, GET_FUNC_PTR( ORIG_ClientKill ) );
	m_hClientPutInServer = DetoursAPI()->DetourFunction( g_pServerFuncs->pfnClientPutInServer, HOOKED_ClientPutInServer, GET_FUNC_PTR( ORIG_ClientPutInServer ) );
	m_hClientCommand = DetoursAPI()->DetourFunction( g_pServerFuncs->pfnClientCommand, HOOKED_ClientCommand, GET_FUNC_PTR( ORIG_ClientCommand ) );
	m_hPlayerSpawns = DetoursAPI()->DetourFunction( m_pfnPlayerSpawns, HOOKED_PlayerSpawns, GET_FUNC_PTR( ORIG_PlayerSpawns ) );
	m_hFixPlayerStuck = DetoursAPI()->DetourFunction( m_pfnFixPlayerStuck, HOOKED_FixPlayerStuck, GET_FUNC_PTR( ORIG_FixPlayerStuck ) );

	m_hCBasePlayer__SpecialSpawn = DetoursAPI()->DetourVirtualFunction( &dummyBasePlayer,
																		Offsets::BasePlayer::SpecialSpawn,
																		HOOKED_CBasePlayer__SpecialSpawn,
																		GET_FUNC_PTR( ORIG_CBasePlayer__SpecialSpawn ) );

	m_hCBasePlayer__BeginRevive = DetoursAPI()->DetourVirtualFunction( &dummyBasePlayer,
																		Offsets::BasePlayer::BeginRevive,
																		HOOKED_CBasePlayer__BeginRevive,
																		GET_FUNC_PTR( ORIG_CBasePlayer__BeginRevive ) );

	m_hCBasePlayer__EndRevive = DetoursAPI()->DetourVirtualFunction( &dummyBasePlayer,
																		Offsets::BasePlayer::EndRevive,
																		HOOKED_CBasePlayer__EndRevive,
																		GET_FUNC_PTR( ORIG_CBasePlayer__EndRevive ) );

	m_hCopyPEntityVars = DetoursAPI()->DetourFunction( MemoryUtils()->CalcAbsoluteAddress( m_pfnCopyPEntityVars ), HOOKED_CopyPEntityVars, GET_FUNC_PTR( ORIG_CopyPEntityVars ) );
	m_hFireTargets = DetoursAPI()->DetourFunction( m_pfnFireTargets, HOOKED_FireTargets, GET_FUNC_PTR( ORIG_FireTargets ) );
}

//-----------------------------------------------------------------------------
// Shutdown server's library
//-----------------------------------------------------------------------------

void CServerModule::Shutdown( void )
{
	DetoursAPI()->RemoveDetour( m_hUse );
	DetoursAPI()->RemoveDetour( m_hTouch );
	DetoursAPI()->RemoveDetour( m_hClientKill );
	DetoursAPI()->RemoveDetour( m_hClientPutInServer );
	DetoursAPI()->RemoveDetour( m_hClientCommand );
	DetoursAPI()->RemoveDetour( m_hPlayerSpawns );
	DetoursAPI()->RemoveDetour( m_hFixPlayerStuck );

	DetoursAPI()->RemoveDetour( m_hCBasePlayer__SpecialSpawn );
	DetoursAPI()->RemoveDetour( m_hCBasePlayer__BeginRevive );
	DetoursAPI()->RemoveDetour( m_hCBasePlayer__EndRevive );

	DetoursAPI()->RemoveDetour( m_hCopyPEntityVars );
	DetoursAPI()->RemoveDetour( m_hFireTargets );
}