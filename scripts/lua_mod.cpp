#include "lua_mod.h"
#include "lua_vector.h"
#include "lua_entity_dictionary.h"
#include "scripts.h"

#include "../modules/server.h"
#include "../features/speedrun_tools.h"

#include <string>

#include <dbg.h>
#include <ISvenModAPI.h>
#include <hl_sdk/engine/APIProxy.h>

static char mapname_buffer[ MAX_PATH ];

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

	lua_pushstring( pLuaState, pszMapName );

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

static int ScriptFunc_GetEntityIndexFromEdict( lua_State *pLuaState )
{
	edict_t *pEdict = lua_getedict( pLuaState, 1 );

	lua_pushinteger( pLuaState, (lua_Integer)g_pServerEngineFuncs->pfnIndexOfEdict( pEdict ) );

	return 1;
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

//-----------------------------------------------------------------------------
// Init lib
//-----------------------------------------------------------------------------

LUALIB_API int luaopen_mod( lua_State *pLuaState )
{
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

	// GetEntityIndexFromEdict
	lua_pushcfunction( pLuaState, ScriptFunc_GetEntityIndexFromEdict );
	lua_setglobal( pLuaState, "GetEntityIndexFromEdict" );
	
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

	// SegmentCurrentTime
	lua_pushcfunction( pLuaState, ScriptFunc_SegmentCurrentTime );
	lua_setglobal( pLuaState, "SegmentCurrentTime" );

	// LookAt
	lua_pushcfunction( pLuaState, ScriptFunc_LookAt );
	lua_setglobal( pLuaState, "LookAt" );

	return 1;
}