#include "lua_mod.h"
#include "lua_vector.h"
#include "lua_usercmd.h"
#include "lua_entity_dictionary.h"
#include "scripts.h"

#include "../modules/server.h"
#include "../modules/server_client_bridge.h"

#include "../features/aim.h"
#include "../features/misc.h"
#include "../features/speedrun_tools.h"

#include <string>

#include <dbg.h>
#include <ISvenModAPI.h>
#include <hl_sdk/engine/APIProxy.h>

//-----------------------------------------------------------------------------
// Get mapname helper
//-----------------------------------------------------------------------------

static const char *GetMapName()
{
	static char mapname_buffer[ MAX_PATH ];

	char *pszMapName = mapname_buffer;
	char *pszExt = NULL;

	strncpy( mapname_buffer, g_pEngineFuncs->GetLevelName(), MAX_PATH );

	// maps/<mapname>.bsp to <mapname>
	while ( *pszMapName )
	{
		if ( *pszMapName == '/' )
		{
			pszMapName++;
			break;
		}

		pszMapName++;
	}

	pszExt = pszMapName;

	while ( *pszExt )
	{
		if ( *pszExt == '.' )
		{
			*pszExt = 0;
			break;
		}

		pszExt++;
	}

	char *tmp = pszMapName;
	while ( *tmp )
	{
		*tmp = tolower( *tmp );
		tmp++;
	}

	return pszMapName;
}

//-----------------------------------------------------------------------------
// C to Lua
//-----------------------------------------------------------------------------

static int ScriptFunc_IncludeScript( lua_State *pLuaState )
{
	const char *pszFilename = lua_tostring( pLuaState, 1 );

	auto ends_with = []( std::string const &value, std::string const &ending ) -> bool
	{
		if ( ending.size() > value.size() )
			return false;

		return std::equal( ending.rbegin(), ending.rend(), value.rbegin() );
	};

	std::string sFilePath = SvenModAPI()->GetBaseDirectory();

	sFilePath += "\\sven_internal\\scripts\\";
	sFilePath += pszFilename;

	if ( !ends_with( pszFilename, ".lua" ) )
	{
		sFilePath += ".lua";
	}

	g_ScriptVM.RunScriptFile( sFilePath.c_str() );

	return 0;
}

static int ScriptFunc_GetMapName( lua_State *pLuaState )
{
	lua_pushstring( pLuaState, GetMapName() );

	return 1;
}

static int ScriptFunc_ClientCmd( lua_State *pLuaState )
{
	const char *pszConCommand = lua_tostring( pLuaState, 1 );
	g_pEngineFuncs->ClientCmd( pszConCommand );

	return 0;
}

static int ScriptFunc_PrintChatText( lua_State *pLuaState )
{
	static char buffer[ 1024 ];

	const char *pszText = lua_tostring( pLuaState, 1 );

	strncpy( buffer, pszText, 1023 );
	buffer[ 1023 ] = 0;

	Utils()->PrintChatText( pszText );

	return 0;
}

static int ScriptFunc_GetPEntityFromEntityIndex( lua_State *pLuaState )
{
	if ( !Host_IsServerActive() )
	{
		lua_pushnil( pLuaState );
		return 1;
	}

	int iEntIndex = (int)lua_tointeger( pLuaState, 1 );

	edict_t *pEdict = g_pServerEngineFuncs->pfnPEntityOfEntIndex( iEntIndex );

	if ( pEdict != NULL )
	{
		lua_pushedict( pLuaState, pEdict );
	}
	else
	{
		lua_pushnil( pLuaState );
	}

	return 1;
}

static int ScriptFunc_GetEntityIndexFromEdict( lua_State *pLuaState )
{
	if ( !Host_IsServerActive() )
	{
		lua_pushinteger( pLuaState, -1 );
		return 1;
	}

	edict_t *pEdict = lua_getedict( pLuaState, 1 );

	lua_pushinteger( pLuaState, (lua_Integer)g_pServerEngineFuncs->pfnIndexOfEdict( pEdict ) );

	return 1;
}

static int ScriptFunc_SendCommandToClient( lua_State *pLuaState )
{
	if ( !Host_IsServerActive() )
		return 0;

	edict_t *ed = lua_getedict( pLuaState, 1 );
	const char *command = lua_tostring( pLuaState, 2 );

	g_pServerEngineFuncs->pfnMessageBegin( MSG_ONE, SVC_SVENINT, NULL, ed );
		g_pServerEngineFuncs->pfnWriteByte( SVENINT_COMM_EXECUTE );
		g_pServerEngineFuncs->pfnWriteString( command );
	g_pServerEngineFuncs->pfnMessageEnd();

	return 0;
}

static int ScriptFunc_SendSignalToClient( lua_State *pLuaState )
{
	if ( !Host_IsServerActive() )
		return 0;

	edict_t *ed = lua_getedict( pLuaState, 1 );
	int value = (int)lua_tointeger( pLuaState, 2 );

	g_pServerEngineFuncs->pfnMessageBegin( MSG_ONE, SVC_SVENINT, NULL, ed );
		g_pServerEngineFuncs->pfnWriteByte( SVENINT_COMM_SCRIPTS );
		g_pServerEngineFuncs->pfnWriteByte( 0 ); // signal from server
		g_pServerEngineFuncs->pfnWriteLong( value );
	g_pServerEngineFuncs->pfnMessageEnd();

	return 0;
}

static int ScriptFunc_IsListenServer( lua_State *pLuaState )
{
	lua_pushboolean( pLuaState, (int)Host_IsServerActive() );

	return 1;
}

static int ScriptFunc_IsSurvivalModeEnabled( lua_State *pLuaState )
{
	lua_pushboolean( pLuaState, (int)IsSurvivalModeEnabled() );

	return 1;
}

static int ScriptFunc_EnableSurvivalMode( lua_State *pLuaState )
{
	lua_pushboolean( pLuaState, (int)EnableSurvivalMode() );

	return 1;
}

static int ScriptFunc_DisableSurvivalMode( lua_State *pLuaState )
{
	lua_pushboolean( pLuaState, (int)DisableSurvivalMode() );

	return 1;
}

static int ScriptFunc_SetTimescale( lua_State *pLuaState )
{
	float timescale = (float)lua_tonumber( pLuaState, 1 );

	g_SpeedrunTools.SetTimescale( timescale );

	return 0;
}

static int ScriptFunc_StartTimer( lua_State *pLuaState )
{
	g_SpeedrunTools.StartTimer();

	return 0;
}

static int ScriptFunc_StopTimer( lua_State *pLuaState )
{
	g_SpeedrunTools.StopTimer();

	return 0;
}

static int ScriptFunc_SegmentCurrentTime( lua_State *pLuaState )
{
	lua_pushnumber( pLuaState, (lua_Number)g_SpeedrunTools.SegmentCurrentTime() );

	return 1;
}

static int ScriptFunc_LookAt( lua_State *pLuaState )
{
	Vector vecAngles, vecDir;

	Vector *vecPoint = lua_getvector( pLuaState, 1 );

	vecDir = *vecPoint - ( Client()->GetOrigin() + Client()->GetViewOffset() );

	vecAngles.x = -atan2f( vecDir.z, vecDir.Length2D() ) * (float)( 180.0 / M_PI );
	vecAngles.y = atan2f( vecDir.y, vecDir.x ) * (float)( 180.0 / M_PI );
	vecAngles.z = 0.f;

	g_pEngineFuncs->SetViewAngles( vecAngles );

	return 0;
}

static int ScriptFunc_SetViewAngles( lua_State *pLuaState )
{
	Vector *va = lua_getvector( pLuaState, 1 );

	g_pEngineFuncs->SetViewAngles( *va );

	return 0;
}

static int ScriptFunc_GetViewAngles( lua_State *pLuaState )
{
	Vector va;

	g_pEngineFuncs->GetViewAngles( va );

	lua_newvector( pLuaState, &va );

	return 1;
}

static int ScriptFunc_Aimbot( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );
	bool bAimbot = lua_toboolean( pLuaState, 2 );
	bool bSilentAimbot = lua_toboolean( pLuaState, 3 );
	bool bRagebot = lua_toboolean( pLuaState, 4 );
	bool bChangeAnglesBack = lua_toboolean( pLuaState, 5 );

	bool bAnglesChanged = false;

	g_Aim.Aimbot( cmd, bAimbot, bSilentAimbot, bRagebot, bChangeAnglesBack, bAnglesChanged );

	if ( !bAnglesChanged && g_Misc.m_bSpinnerDelayed )
	{
		g_Misc.Spinner( cmd );
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Init lib
//-----------------------------------------------------------------------------

void lua_setcurrentmap( lua_State *pLuaState )
{
	// Current map name
	lua_pushstring( pLuaState, GetMapName() );
	lua_setglobal( pLuaState, "MapName" );
}

LUALIB_API int luaopen_mod( lua_State *pLuaState )
{
	lua_setcurrentmap( pLuaState );

	// Max clients
	lua_pushinteger( pLuaState, (lua_Integer)gpGlobals->maxClients );
	lua_setglobal( pLuaState, "MaxClients" );

	// Client state
	lua_pushinteger( pLuaState, client_state_t::CLS_NONE );
	lua_setglobal( pLuaState, "CLS_NONE" );

	lua_pushinteger( pLuaState, client_state_t::CLS_DISCONNECTED );
	lua_setglobal( pLuaState, "CLS_DISCONNECTED" );

	lua_pushinteger( pLuaState, client_state_t::CLS_CHALLENGE );
	lua_setglobal( pLuaState, "CLS_CHALLENGE" );

	lua_pushinteger( pLuaState, client_state_t::CLS_CONNECTED );
	lua_setglobal( pLuaState, "CLS_CONNECTED" );

	lua_pushinteger( pLuaState, client_state_t::CLS_LOADING );
	lua_setglobal( pLuaState, "CLS_LOADING" );

	lua_pushinteger( pLuaState, client_state_t::CLS_ACTIVE );
	lua_setglobal( pLuaState, "CLS_ACTIVE" );

	// Input buttons
	lua_pushinteger( pLuaState, IN_ATTACK );
	lua_setglobal( pLuaState, "IN_ATTACK" );
	
	lua_pushinteger( pLuaState, IN_JUMP );
	lua_setglobal( pLuaState, "IN_JUMP" );
	
	lua_pushinteger( pLuaState, IN_DUCK );
	lua_setglobal( pLuaState, "IN_DUCK" );
	
	lua_pushinteger( pLuaState, IN_FORWARD );
	lua_setglobal( pLuaState, "IN_FORWARD" );
	
	lua_pushinteger( pLuaState, IN_BACK );
	lua_setglobal( pLuaState, "IN_BACK" );
	
	lua_pushinteger( pLuaState, IN_USE );
	lua_setglobal( pLuaState, "IN_USE" );
	
	lua_pushinteger( pLuaState, IN_CANCEL );
	lua_setglobal( pLuaState, "IN_CANCEL" );
	
	lua_pushinteger( pLuaState, IN_LEFT );
	lua_setglobal( pLuaState, "IN_LEFT" );
	
	lua_pushinteger( pLuaState, IN_RIGHT );
	lua_setglobal( pLuaState, "IN_RIGHT" );
	
	lua_pushinteger( pLuaState, IN_MOVELEFT );
	lua_setglobal( pLuaState, "IN_MOVELEFT" );
	
	lua_pushinteger( pLuaState, IN_MOVERIGHT );
	lua_setglobal( pLuaState, "IN_MOVERIGHT" );
	
	lua_pushinteger( pLuaState, IN_ATTACK2 );
	lua_setglobal( pLuaState, "IN_ATTACK2" );
	
	lua_pushinteger( pLuaState, IN_RUN );
	lua_setglobal( pLuaState, "IN_RUN" );
	
	lua_pushinteger( pLuaState, IN_RELOAD );
	lua_setglobal( pLuaState, "IN_RELOAD" );
	
	lua_pushinteger( pLuaState, IN_ALT1 );
	lua_setglobal( pLuaState, "IN_ALT1" );
	
	lua_pushinteger( pLuaState, IN_SCORE );
	lua_setglobal( pLuaState, "IN_SCORE" );
	
	// Flags
	lua_pushinteger( pLuaState, FL_ONGROUND );
	lua_setglobal( pLuaState, "FL_ONGROUND" );

	lua_pushinteger( pLuaState, FL_DUCKING );
	lua_setglobal( pLuaState, "FL_DUCKING" );

	// IncludeScript
	lua_pushcfunction( pLuaState, ScriptFunc_IncludeScript );
	lua_setglobal( pLuaState, "IncludeScript" );

	// GetMapName
	lua_pushcfunction( pLuaState, ScriptFunc_GetMapName );
	lua_setglobal( pLuaState, "GetMapName" );

	// ClientCmd
	lua_pushcfunction( pLuaState, ScriptFunc_ClientCmd );
	lua_setglobal( pLuaState, "ClientCmd" );

	// PrintChatText
	lua_pushcfunction( pLuaState, ScriptFunc_PrintChatText );
	lua_setglobal( pLuaState, "PrintChatText" );

	// GetPEntityFromEntityIndex
	lua_pushcfunction( pLuaState, ScriptFunc_GetPEntityFromEntityIndex );
	lua_setglobal( pLuaState, "GetPEntityFromEntityIndex" );
	
	// GetEntityIndexFromEdict
	lua_pushcfunction( pLuaState, ScriptFunc_GetEntityIndexFromEdict );
	lua_setglobal( pLuaState, "GetEntityIndexFromEdict" );
	
	// SendCommandToClient
	lua_pushcfunction( pLuaState, ScriptFunc_SendCommandToClient );
	lua_setglobal( pLuaState, "SendCommandToClient" );
	
	// SendSignalToClient
	lua_pushcfunction( pLuaState, ScriptFunc_SendSignalToClient );
	lua_setglobal( pLuaState, "SendSignalToClient" );
	
	// IsListenServer
	lua_pushcfunction( pLuaState, ScriptFunc_IsListenServer );
	lua_setglobal( pLuaState, "IsListenServer" );

	// IsSurvivalModeEnabled
	lua_pushcfunction( pLuaState, ScriptFunc_IsSurvivalModeEnabled );
	lua_setglobal( pLuaState, "IsSurvivalModeEnabled" );
	
	// EnableSurvivalMode
	lua_pushcfunction( pLuaState, ScriptFunc_EnableSurvivalMode );
	lua_setglobal( pLuaState, "EnableSurvivalMode" );

	// DisableSurvivalMode
	lua_pushcfunction( pLuaState, ScriptFunc_DisableSurvivalMode );
	lua_setglobal( pLuaState, "DisableSurvivalMode" );

	// SetTimescale
	lua_pushcfunction( pLuaState, ScriptFunc_SetTimescale );
	lua_setglobal( pLuaState, "SetTimescale" );
	
	// StartTimer
	lua_pushcfunction( pLuaState, ScriptFunc_StartTimer );
	lua_setglobal( pLuaState, "StartTimer" );
	
	// StopTimer
	lua_pushcfunction( pLuaState, ScriptFunc_StopTimer );
	lua_setglobal( pLuaState, "StopTimer" );
	
	// SegmentCurrentTime
	lua_pushcfunction( pLuaState, ScriptFunc_SegmentCurrentTime );
	lua_setglobal( pLuaState, "SegmentCurrentTime" );

	// LookAt
	lua_pushcfunction( pLuaState, ScriptFunc_LookAt );
	lua_setglobal( pLuaState, "LookAt" );
	
	// SetViewAngles
	lua_pushcfunction( pLuaState, ScriptFunc_SetViewAngles );
	lua_setglobal( pLuaState, "SetViewAngles" );
	
	// GetViewAngles
	lua_pushcfunction( pLuaState, ScriptFunc_GetViewAngles );
	lua_setglobal( pLuaState, "GetViewAngles" );
	
	// Aimbot
	lua_pushcfunction( pLuaState, ScriptFunc_Aimbot );
	lua_setglobal( pLuaState, "Aimbot" );

	return 1;
}