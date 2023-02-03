#include "scripts.h"
#include "lua_debug.h"
#include "lua_vector.h"
#include "lua_cvar.h"
#include "lua_mod.h"
#include "lua_logic.h"
#include "lua_triggers.h"

#include <string>

#include <ISvenModAPI.h>

#include <dbg.h>
#include <convar.h>

static const Color clr_print(80, 186, 255, 255);

//-----------------------------------------------------------------------------
// ConCommands
//-----------------------------------------------------------------------------

CON_COMMAND(sc_script, "Execute a script line")
{
	if ( args.ArgC() > 1 )
	{
		if ( g_ScriptVM.GetVM() == NULL )
		{
			Msg("Scripts virtual machine is not running\n");
			return;
		}

		if ( args.ArgC() > 2 )
		{
			std::string sScript = args[1];

			for (int i = 2; i < args.ArgC(); i++)
			{
				sScript += " ";
				sScript += args[i];
			}

			g_ScriptVM.RunScript( sScript.c_str() );
		}
		else
		{
			g_ScriptVM.RunScript( args[1] );
		}
	}
	else
	{
		Msg("Usage:  sc_script <script>\n");
	}
}

CON_COMMAND(sc_script_execute, "Execute a script file")
{
	if (args.ArgC() > 1)
	{
		if ( g_ScriptVM.GetVM() == NULL )
		{
			Msg("Scripts virtual machine is not running\n");
			return;
		}

		//g_ScriptVM.RunScriptFile( args[1] );

		auto ends_with = [](std::string const &value, std::string const &ending) -> bool
		{
			if ( ending.size() > value.size() )
				return false;

			return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
		};

		const char *pszFilename = args[1];

		std::string sFilePath = SvenModAPI()->GetBaseDirectory();

		sFilePath += "\\sven_internal\\scripts\\";
		sFilePath += pszFilename;

		if ( !ends_with(pszFilename, ".lua") )
		{
			sFilePath += ".lua";
		}

		g_ScriptVM.RunScriptFile( sFilePath.c_str() );
	}
	else
	{
		Msg("Usage:  sc_script_execute <filename>\n");
	}
}

//-----------------------------------------------------------------------------
// Scripts Callbacks
//-----------------------------------------------------------------------------

void CScriptCallbacks::OnFirstClientdataReceived(float flTime)
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	if ( pLuaState != NULL )
	{
		if ( g_ScriptVM.LookupFunction("OnFirstClientdataReceived") )
		{
			lua_getglobal(pLuaState, "OnFirstClientdataReceived");

			lua_pushnumber(pLuaState, (lua_Number)flTime);

			lua_call(pLuaState, 1, 0);
		}
	}
}

void CScriptCallbacks::OnBeginLoading(void)
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	if ( pLuaState != NULL )
	{
		g_TimersHandler.ClearTimers();
		g_ClientTriggerManager.ClearTriggers();

		if ( g_ScriptVM.LookupFunction("OnBeginLoading") )
		{
			lua_getglobal(pLuaState, "OnBeginLoading");
			lua_call(pLuaState, 0, 0);
		}
	}
}

void CScriptCallbacks::OnEndLoading(void)
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	if ( pLuaState != NULL )
	{
		g_TimersHandler.ClearTimers();
		g_ClientTriggerManager.ClearTriggers();

		if ( g_ScriptVM.LookupFunction("OnEndLoading") )
		{
			lua_getglobal(pLuaState, "OnEndLoading");
			lua_call(pLuaState, 0, 0);
		}
	}
}

void CScriptCallbacks::OnDisconnect(void)
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	if ( pLuaState != NULL )
	{
		if ( g_ScriptVM.LookupFunction("OnDisconnect") )
		{
			lua_getglobal(pLuaState, "OnDisconnect");
			lua_call(pLuaState, 0, 0);
		}
	}
}

CScriptCallbacks g_ScriptCallbacks;

//-----------------------------------------------------------------------------
// Lua's Script VM
//-----------------------------------------------------------------------------

bool CScriptVM::Init(void)
{
	if ( m_pLuaState == NULL )
	{
		m_pLuaState = luaL_newstate();

		if ( m_pLuaState != NULL )
		{
			luaL_openlibs(m_pLuaState);

			luaopen_print(m_pLuaState);
			luaopen_vector(m_pLuaState);
			luaopen_cvar(m_pLuaState);
			luaopen_mod(m_pLuaState);
			luaopen_logic(m_pLuaState);
			luaopen_triggers(m_pLuaState);

			//SetSearchPath("sven_internal\\scripts");

			ConColorMsg(clr_print, "Started scripts virtual machine using scripting language \"" LUA_VERSION "\"\n");

			ConColorMsg(clr_print, "Running a script file \"main.lua\"...\n");
			RunScriptFile("sven_internal/scripts/main.lua");

			return true;
		}

		Warning("Failed to start scripts virtual machine\n");
	}
	else
	{
		// VM already exists
		g_TimersHandler.ClearTimers();
		g_ClientTriggerManager.ClearTriggers();
	}

	return false;
}

void CScriptVM::Shutdown(void)
{
	if ( m_pLuaState != NULL )
	{
		lua_close( m_pLuaState );
		m_pLuaState = NULL;

		ConColorMsg(clr_print, "Stopped scripts virtual machine\n");
	}
}

void CScriptVM::Frame(client_state_t state, double frametime, bool bPostRunCmd)
{
	if ( m_pLuaState == NULL )
		return;

	if ( bPostRunCmd )
	{
		if ( LookupFunction("OnGameFrame") )
		{
			lua_getglobal(m_pLuaState, "OnGameFrame");

			lua_pushinteger(m_pLuaState, (lua_Integer)state);
			lua_pushnumber(m_pLuaState, (lua_Number)frametime);

			int luaResult = lua_pcall(m_pLuaState, 2, 0, 0);

			if ( luaResult != LUA_OK )
			{
				g_ScriptVM.PrintError();
			}
		}
	}
	else
	{
		g_TimersHandler.Frame( m_pLuaState );
		g_ClientTriggerManager.Frame( m_pLuaState );
	}
}

lua_State *CScriptVM::GetVM(void)
{
	return m_pLuaState;
}

void CScriptVM::SetSearchPath(const char *pszSearchPath)
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

	lua_getglobal(m_pLuaState, "package");

	if ( lua_istable(m_pLuaState, -1) )
	{
		lua_getfield(m_pLuaState, -1, "path");

		if ( lua_isstring(m_pLuaState, -1) )
		{
			std::string sPath = SvenModAPI()->GetBaseDirectory();

			sPath += "\\";
			sPath += pszSearchPath;
			sPath += "\\?.lua";
			sPath += ";.\\";
			sPath += pszSearchPath;
			sPath += "\\?.lua";

			lua_pushstring(m_pLuaState, sPath.c_str());
			lua_setfield(m_pLuaState, -3, "path");

			lua_pop(m_pLuaState, 2);
		}
		else
		{
			AssertMsg( 0, "path" );
			lua_pop(m_pLuaState, 1);
		}
	}
	else
	{
		AssertMsg( 0, "package" );
		lua_pop(m_pLuaState, 1);
	}
}

bool CScriptVM::RunScript(const char *pszScript)
{
	if ( m_pLuaState == NULL )
		return false;

	int luaResult = luaL_dostring(m_pLuaState, pszScript);

	if ( luaResult != LUA_OK )
	{
		PrintError();
		DumpStack();

		return false;
	}

	return true;
}

bool CScriptVM::RunScriptFile(const char *pszFilename)
{
	if ( m_pLuaState == NULL )
		return false;

	int luaResult = luaL_dofile(m_pLuaState, pszFilename);

	if ( luaResult != LUA_OK )
	{
		PrintError();
		DumpStack();

		return false;
	}

	return true;
}

bool CScriptVM::LookupFunction(const char *pszFunction)
{
	if ( m_pLuaState != NULL )
	{
		lua_getglobal( m_pLuaState, pszFunction );

		if ( lua_isfunction(m_pLuaState, -1) || lua_iscfunction(m_pLuaState, -1) )
		{
			lua_pop(m_pLuaState, 1);
			return true;
		}

		lua_pop(m_pLuaState, 1);
	}

	return false;
}

//-----------------------------------------------------------------------------
// Private methods
//-----------------------------------------------------------------------------

void CScriptVM::PrintError(void)
{
	Warning("\nAN ERROR HAS OCCURRED [ %s ]\n\n", lua_tostring(m_pLuaState, -1));
}

void CScriptVM::DumpStack(void)
{
	Warning("STACK TRACE:\n");

	const int level = 10;

	int top = lua_gettop(m_pLuaState);
	int stack_end = top - level;

	if ( stack_end < 1 )
		stack_end = 1;

	for (int i = top; i >= stack_end; i--)
	{
		int type = lua_type(m_pLuaState, i);

		Warning("%d. ", i);

		switch (type)
		{
		case LUA_TNUMBER:
			Warning("%g", lua_tonumber(m_pLuaState, i));
			break;

		case LUA_TSTRING:
			Warning("`%s'", lua_tostring(m_pLuaState, i));
			break;

		case LUA_TBOOLEAN:
			Warning(lua_toboolean(m_pLuaState, i) ? "true" : "false");
			break;

		case LUA_TNIL:
			Warning("nil");
			break;

		default:
			Warning("%s", lua_typename(m_pLuaState, type));
			break;
		}

		Warning("\n\n");
	}
}

//-----------------------------------------------------------------------------
// Set globals
//-----------------------------------------------------------------------------

CScriptVM g_ScriptVM;
CScriptVM *g_pScriptVM = &g_ScriptVM;