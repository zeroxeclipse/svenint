#include "scripts.h"
#include "lua_debug.h"
#include "lua_vector.h"
#include "lua_cvar.h"
#include "lua_mod.h"
#include "lua_logic.h"
#include "lua_triggers.h"
#include "lua_entity_dictionary.h"
#include "lua_entity_vars.h"
#include "lua_player_move.h"
#include "lua_usercmd.h"
#include "lua_input_manager.h"

#include <string>

#include <ISvenModAPI.h>

#include <dbg.h>
#include <convar.h>

extern bool g_bPlayingbackDemo;

static const Color clr_print( 80, 186, 255, 255 );

//-----------------------------------------------------------------------------
// ConCommands
//-----------------------------------------------------------------------------

CON_COMMAND( sc_script, "Execute a script line" )
{
	if ( args.ArgC() > 1 )
	{
		if ( g_ScriptVM.GetVM() == NULL )
		{
			Msg( "Scripts virtual machine is not running\n" );
			return;
		}

		if ( args.ArgC() > 2 )
		{
			std::string sScript = args[ 1 ];

			for ( int i = 2; i < args.ArgC(); i++ )
			{
				sScript += " ";
				sScript += args[ i ];
			}

			g_ScriptVM.RunScript( sScript.c_str() );
		}
		else
		{
			g_ScriptVM.RunScript( args[ 1 ] );
		}
	}
	else
	{
		Msg( "Usage:  sc_script <script>\n" );
	}
}

CON_COMMAND( sc_script_execute, "Execute a script file" )
{
	if ( args.ArgC() > 1 )
	{
		if ( g_ScriptVM.GetVM() == NULL )
		{
			Msg( "Scripts virtual machine is not running\n" );
			return;
		}

		//g_ScriptVM.RunScriptFile( args[1] );

		auto ends_with = []( std::string const &value, std::string const &ending ) -> bool
		{
			if ( ending.size() > value.size() )
				return false;

			return std::equal( ending.rbegin(), ending.rend(), value.rbegin() );
		};

		const char *pszFilename = args[ 1 ];

		std::string sFilePath = SvenModAPI()->GetBaseDirectory();

		sFilePath += "\\sven_internal\\scripts\\";
		sFilePath += pszFilename;

		if ( !ends_with( pszFilename, ".lua" ) )
		{
			sFilePath += ".lua";
		}

		g_ScriptVM.RunScriptFile( sFilePath.c_str() );
	}
	else
	{
		Msg( "Usage:  sc_script_execute <filename>\n" );
	}
}

//-----------------------------------------------------------------------------
// Scripts Callbacks
//-----------------------------------------------------------------------------

void CScriptCallbacks::OnGameFrame( client_state_t state, double frametime, bool bPostRunCmd )
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	if ( pLuaState != NULL )
	{
		scriptref_t hFunction;

		if ( hFunction = g_ScriptVM.LookupFunction( "OnGameFrame" ) )
		{
			lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, hFunction );

			lua_pushinteger( pLuaState, (lua_Integer)state );
			lua_pushnumber( pLuaState, (lua_Number)frametime );
			lua_pushboolean( pLuaState, bPostRunCmd );

			g_ScriptVM.ProtectedCall( pLuaState, 3, 0, 0 );
			g_ScriptVM.ReleaseFunction( hFunction );
		}
	}
}

void CScriptCallbacks::OnFirstClientdataReceived( float flTime )
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	if ( pLuaState != NULL )
	{
		scriptref_t hFunction;

		if ( hFunction = g_ScriptVM.LookupFunction( "OnFirstClientdataReceived" ) )
		{
			lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, hFunction );

			lua_pushnumber( pLuaState, (lua_Number)flTime );

			g_ScriptVM.ProtectedCall( pLuaState, 1, 0, 0 );
			g_ScriptVM.ReleaseFunction( hFunction );
		}
	}
}

void CScriptCallbacks::OnBeginLoading( void )
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	if ( pLuaState != NULL )
	{
		scriptref_t hFunction;

		g_ScriptVM.ResetStates();

		if ( hFunction = g_ScriptVM.LookupFunction( "OnBeginLoading" ) )
		{
			lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, hFunction );

			g_ScriptVM.ProtectedCall( pLuaState, 0, 0, 0 );
			g_ScriptVM.ReleaseFunction( hFunction );
		}
	}
}

void CScriptCallbacks::OnEndLoading( void )
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	if ( pLuaState != NULL )
	{
		scriptref_t hFunction;

		g_ScriptVM.ResetStates();

		if ( hFunction = g_ScriptVM.LookupFunction( "OnEndLoading" ) )
		{
			lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, hFunction );

			g_ScriptVM.ProtectedCall( pLuaState, 0, 0, 0 );
			g_ScriptVM.ReleaseFunction( hFunction );
		}
	}
}

void CScriptCallbacks::OnDisconnect( void )
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	if ( pLuaState != NULL )
	{
		scriptref_t hFunction;

		if ( hFunction = g_ScriptVM.LookupFunction( "OnDisconnect" ) )
		{
			lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, hFunction );

			g_ScriptVM.ProtectedCall( pLuaState, 0, 0, 0 );
			g_ScriptVM.ReleaseFunction( hFunction );
		}
	}
}

// Server-specific callbacks

void CScriptCallbacks::OnRestart( void )
{
	//lua_State *pLuaState = g_ScriptVM.GetVM();

	//if ( pLuaState != NULL )
	//{
	//	scriptref_t hFunction;

	//	g_ScriptVM.ResetStates();

	//	if ( hFunction = g_ScriptVM.LookupFunction("OnRestart") )
	//	{
	//		lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, hFunction );

	//		g_ScriptVM.ProtectedCall( pLuaState, 0, 0, 0 );
	//		g_ScriptVM.ReleaseFunction( hFunction );
	//	}
	//}

	g_ScriptVM.Shutdown();
	g_ScriptVM.Init();
}

void CScriptCallbacks::OnEntityUse( edict_t *pEntityUseEdict, edict_t *pEntityEdict )
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	if ( pLuaState != NULL )
	{
		scriptref_t hFunction;

		if ( hFunction = g_ScriptVM.LookupFunction( "OnEntityUse" ) )
		{
			lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, hFunction );

			lua_pushedict( pLuaState, pEntityUseEdict );
			lua_pushedict( pLuaState, pEntityEdict );

			g_ScriptVM.ProtectedCall( pLuaState, 2, 0, 0 );
			g_ScriptVM.ReleaseFunction( hFunction );
		}
	}
}

void CScriptCallbacks::OnEntityTouch( edict_t *pEntityTouchEdict, edict_t *pEntityEdict )
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	if ( pLuaState != NULL )
	{
		scriptref_t hFunction;

		if ( hFunction = g_ScriptVM.LookupFunction( "OnEntityTouch" ) )
		{
			lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, hFunction );

			lua_pushedict( pLuaState, pEntityTouchEdict );
			lua_pushedict( pLuaState, pEntityEdict );

			g_ScriptVM.ProtectedCall( pLuaState, 2, 0, 0 );
			g_ScriptVM.ReleaseFunction( hFunction );
		}
	}
}

void CScriptCallbacks::OnClientPutInServer( edict_t *pPlayerEdict )
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	if ( pLuaState != NULL )
	{
		scriptref_t hFunction;

		if ( hFunction = g_ScriptVM.LookupFunction( "OnClientPutInServer" ) )
		{
			lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, hFunction );

			lua_pushedict( pLuaState, pPlayerEdict );

			g_ScriptVM.ProtectedCall( pLuaState, 1, 0, 0 );
			g_ScriptVM.ReleaseFunction( hFunction );
		}
	}
}

void CScriptCallbacks::OnPlayerSpawn( edict_t *pSpawnSpotEdict, edict_t *pPlayerEdict )
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	if ( pLuaState != NULL )
	{
		scriptref_t hFunction;

		if ( hFunction = g_ScriptVM.LookupFunction( "OnPlayerSpawn" ) )
		{
			lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, hFunction );

			lua_pushedict( pLuaState, pSpawnSpotEdict );
			lua_pushedict( pLuaState, pPlayerEdict );

			g_ScriptVM.ProtectedCall( pLuaState, 2, 0, 0 );
			g_ScriptVM.ReleaseFunction( hFunction );
		}
	}
}

void CScriptCallbacks::OnPlayerUnstuck( edict_t *pPlayerEdict )
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	if ( pLuaState != NULL )
	{
		scriptref_t hFunction;

		if ( hFunction = g_ScriptVM.LookupFunction( "OnPlayerUnstuck" ) )
		{
			lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, hFunction );

			lua_pushedict( pLuaState, pPlayerEdict );

			g_ScriptVM.ProtectedCall( pLuaState, 1, 0, 0 );
			g_ScriptVM.ReleaseFunction( hFunction );
		}
	}
}

void CScriptCallbacks::OnSpecialSpawn( edict_t *pPlayerEdict )
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	if ( pLuaState != NULL )
	{
		scriptref_t hFunction;

		if ( hFunction = g_ScriptVM.LookupFunction( "OnSpecialSpawn" ) )
		{
			lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, hFunction );

			lua_pushedict( pLuaState, pPlayerEdict );

			g_ScriptVM.ProtectedCall( pLuaState, 1, 0, 0 );
			g_ScriptVM.ReleaseFunction( hFunction );
		}
	}
}

void CScriptCallbacks::OnBeginPlayerRevive( edict_t *pPlayerEdict )
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	if ( pLuaState != NULL )
	{
		scriptref_t hFunction;

		if ( hFunction = g_ScriptVM.LookupFunction( "OnBeginPlayerRevive" ) )
		{
			lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, hFunction );

			lua_pushedict( pLuaState, pPlayerEdict );

			g_ScriptVM.ProtectedCall( pLuaState, 1, 0, 0 );
			g_ScriptVM.ReleaseFunction( hFunction );
		}
	}
}

void CScriptCallbacks::OnEndPlayerRevive( edict_t *pPlayerEdict )
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	if ( pLuaState != NULL )
	{
		scriptref_t hFunction;

		if ( hFunction = g_ScriptVM.LookupFunction( "OnEndPlayerRevive" ) )
		{
			lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, hFunction );

			lua_pushedict( pLuaState, pPlayerEdict );

			g_ScriptVM.ProtectedCall( pLuaState, 1, 0, 0 );
			g_ScriptVM.ReleaseFunction( hFunction );
		}
	}
}

void CScriptCallbacks::OnClientKill( edict_t *pPlayerEdict )
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	if ( pLuaState != NULL )
	{
		scriptref_t hFunction;

		if ( hFunction = g_ScriptVM.LookupFunction( "OnClientKill" ) )
		{
			lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, hFunction );

			lua_pushedict( pLuaState, pPlayerEdict );

			g_ScriptVM.ProtectedCall( pLuaState, 1, 0, 0 );
			g_ScriptVM.ReleaseFunction( hFunction );
		}
	}
}

void CScriptCallbacks::OnServerSignal( int value )
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	if ( pLuaState != NULL )
	{
		scriptref_t hFunction;

		if ( hFunction = g_ScriptVM.LookupFunction( "OnServerSignal" ) )
		{
			lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, hFunction );

			lua_pushinteger( pLuaState, value );

			g_ScriptVM.ProtectedCall( pLuaState, 1, 0, 0 );
			g_ScriptVM.ReleaseFunction( hFunction );
		}
	}
}

// Input Manager callbacks

void CScriptCallbacks::OnPlayInput( const char *pszFilename, int frame, usercmd_t *cmd )
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	if ( pLuaState != NULL )
	{
		scriptref_t hFunction;

		if ( hFunction = g_ScriptVM.LookupFunction( "OnPlayInput" ) )
		{
			lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, hFunction );

			lua_pushstring( pLuaState, pszFilename );
			lua_pushinteger( pLuaState, (lua_Integer)frame );
			lua_pushusercmd( pLuaState, cmd );

			g_ScriptVM.ProtectedCall( pLuaState, 3, 0, 0 );
			g_ScriptVM.ReleaseFunction( hFunction );
		}
	}
}

void CScriptCallbacks::OnPlayEnd( const char *pszFilename, int frames )
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	if ( pLuaState != NULL )
	{
		scriptref_t hFunction;

		if ( hFunction = g_ScriptVM.LookupFunction( "OnPlayEnd" ) )
		{
			lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, hFunction );

			lua_pushstring( pLuaState, pszFilename );
			lua_pushinteger( pLuaState, (lua_Integer)frames );

			g_ScriptVM.ProtectedCall( pLuaState, 2, 0, 0 );
			g_ScriptVM.ReleaseFunction( hFunction );
		}
	}
}

CScriptCallbacks g_ScriptCallbacks;

//-----------------------------------------------------------------------------
// Lua's Script VM
//-----------------------------------------------------------------------------

bool CScriptVM::Init( void )
{
	if ( g_bPlayingbackDemo )
		return false;

	if ( m_pLuaState == NULL )
	{
		m_pLuaState = luaL_newstate();

		if ( m_pLuaState != NULL )
		{
			luaL_openlibs( m_pLuaState );

			luaopen_print( m_pLuaState );
			luaopen_vector( m_pLuaState );
			luaopen_cvar( m_pLuaState );
			luaopen_mod( m_pLuaState );
			luaopen_logic( m_pLuaState );
			luaopen_triggers( m_pLuaState );
			luaopen_edict( m_pLuaState );
			luaopen_entvars( m_pLuaState );
			luaopen_playermove( m_pLuaState );
			luaopen_usercmd( m_pLuaState );
			luaopen_inputmanager( m_pLuaState );

			//SetSearchPath("sven_internal\\scripts");

			ConColorMsg( clr_print, "Started scripts virtual machine using scripting language \"" LUA_VERSION "\"\n" );

			ConColorMsg( clr_print, "Running a script file \"main.lua\"...\n" );
			RunScriptFile( "sven_internal/scripts/main.lua" );

			return true;
		}

		Warning( "Failed to start scripts virtual machine\n" );
	}
	else
	{
		// VM already exists
		ResetStates();
	}

	return false;
}

void CScriptVM::Shutdown( void )
{
	if ( m_pLuaState != NULL )
	{
		lua_close( m_pLuaState );
		m_pLuaState = NULL;

		ConColorMsg( clr_print, "Stopped scripts virtual machine\n" );
	}
}

void CScriptVM::ResetStates( void )
{
	if ( m_pLuaState == NULL )
		return;

	lua_setcurrentmap( m_pLuaState );

	g_TimersHandler.ClearTimers();
	g_LuaTriggerManager.ClearTriggers();
	g_LuaTriggerManager.ClearServerTriggers();
}

void CScriptVM::Frame( client_state_t state, double frametime, bool bPostRunCmd )
{
	if ( m_pLuaState == NULL )
		return;

	if ( !bPostRunCmd )
	{
		// Collect garbage
		lua_gc( m_pLuaState, LUA_GCCOLLECT );

		g_TimersHandler.Frame( m_pLuaState );
		g_LuaTriggerManager.Frame( m_pLuaState );
	}

	g_ScriptCallbacks.OnGameFrame( state, frametime, bPostRunCmd );
}

lua_State *CScriptVM::GetVM( void )
{
	return m_pLuaState;
}

void CScriptVM::SetSearchPath( const char *pszSearchPath )
{

	/*

D:\GAMES\Steam\steamapps\common\Sven Co-op\lua\?.lua
D:\GAMES\Steam\steamapps\common\Sven Co-op\lua\?\init.lua
D:\GAMES\Steam\steamapps\common\Sven Co-op\?.lua
D:\GAMES\Steam\steamapps\common\Sven Co-op\?\init.lua
D:\GAMES\Steam\steamapps\common\Sven Co-op\..\share\lua\5.4\?.lua
D:\GAMES\Steam\steamapps\common\Sven Co-op\..\share\lua\5.4\?\init.lua
.\?.lua
.\?\init.lua

	*/

	lua_getglobal( m_pLuaState, "package" );

	if ( lua_istable( m_pLuaState, -1 ) )
	{
		lua_getfield( m_pLuaState, -1, "path" );

		if ( lua_isstring( m_pLuaState, -1 ) )
		{
			std::string sPath = SvenModAPI()->GetBaseDirectory();

			sPath += "\\";
			sPath += pszSearchPath;
			sPath += "\\?.lua";
			sPath += ";.\\";
			sPath += pszSearchPath;
			sPath += "\\?.lua";

			lua_pushstring( m_pLuaState, sPath.c_str() );
			lua_setfield( m_pLuaState, -3, "path" );

			lua_pop( m_pLuaState, 2 );
		}
		else
		{
			AssertMsg( 0, "path" );
			lua_pop( m_pLuaState, 1 );
		}
	}
	else
	{
		AssertMsg( 0, "package" );
		lua_pop( m_pLuaState, 1 );
	}
}

bool CScriptVM::RunScript( const char *pszScript )
{
	if ( m_pLuaState == NULL )
		return false;

	int luaResult = luaL_dostring( m_pLuaState, pszScript );

	if ( luaResult != LUA_OK )
	{
		PrintError();
		DumpStack();

		return false;
	}

	return true;
}

bool CScriptVM::RunScriptFile( const char *pszFilename )
{
	if ( m_pLuaState == NULL )
		return false;

	int luaResult = luaL_dofile( m_pLuaState, pszFilename );

	if ( luaResult != LUA_OK )
	{
		PrintError();
		DumpStack();

		return false;
	}

	return true;
}

scriptref_t CScriptVM::LookupFunction( const char *pszFunction )
{
	if ( m_pLuaState != NULL )
	{
		lua_getglobal( m_pLuaState, pszFunction );

		if ( !( lua_isfunction( m_pLuaState, -1 ) || lua_iscfunction( m_pLuaState, -1 ) ) )
		{
			lua_pop( m_pLuaState, 1 );
			return NULL;
		}

		scriptref_t func = (scriptref_t)luaL_ref( m_pLuaState, LUA_REGISTRYINDEX );

		return func;
	}

	return NULL;
}

void CScriptVM::ReleaseFunction( scriptref_t hFunction )
{
	if ( m_pLuaState != NULL )
	{
		luaL_unref( m_pLuaState, LUA_REGISTRYINDEX, (int)hFunction );
	}
}

void CScriptVM::ProtectedCall( lua_State *pLuaState, int args, int results, int errfunc )
{
	int luaResult = lua_pcall( pLuaState, args, results, errfunc );

	if ( luaResult != LUA_OK )
	{
		g_ScriptVM.PrintError();
	}
}

//-----------------------------------------------------------------------------
// Private methods
//-----------------------------------------------------------------------------

void CScriptVM::PrintError( void )
{
	Warning( "\nAN ERROR HAS OCCURRED [ %s ]\n\n", lua_tostring( m_pLuaState, -1 ) );
}

void CScriptVM::DumpStack( void )
{
	Warning( "STACK TRACE:\n" );

	const int level = 10;

	int top = lua_gettop( m_pLuaState );
	int stack_end = top - level;

	if ( stack_end < 1 )
		stack_end = 1;

	for ( int i = top; i >= stack_end; i-- )
	{
		int type = lua_type( m_pLuaState, i );

		Warning( "%d. ", i );

		switch ( type )
		{
		case LUA_TNUMBER:
			Warning( "%g", lua_tonumber( m_pLuaState, i ) );
			break;

		case LUA_TSTRING:
			Warning( "`%s'", lua_tostring( m_pLuaState, i ) );
			break;

		case LUA_TBOOLEAN:
			Warning( lua_toboolean( m_pLuaState, i ) ? "true" : "false" );
			break;

		case LUA_TNIL:
			Warning( "nil" );
			break;

		default:
			Warning( "%s", lua_typename( m_pLuaState, type ) );
			break;
		}

		Warning( "\n\n" );
	}
}

//-----------------------------------------------------------------------------
// Set globals
//-----------------------------------------------------------------------------

CScriptVM g_ScriptVM;
CScriptVM *g_pScriptVM = &g_ScriptVM;