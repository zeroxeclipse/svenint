#include "scripts_binding.h"
#include "lua_logic.h"
#include "lua_vector.h"

#include "../game/utils.h"

#include <dbg.h>
#include <convar.h>
#include <IUtils.h>
#include <IRender.h>
#include <hl_sdk/pm_shared/pm_shared.h>

//-----------------------------------------------------------------------------
// CTimersHandler
//-----------------------------------------------------------------------------

CTimersHandler g_TimersHandler;

CTimersHandler::CTimersHandler()
{
	m_ID = 0;
}

void CTimersHandler::Frame(lua_State *pLuaState)
{
	for (size_t i = 0; i < m_vTimers.size(); i++)
	{
		Lua_TimerContext &timer = m_vTimers[i];

		if ( timer.time <= *dbRealtime )
		{
			size_t args = timer.args.size();

			lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, (int)timer.func_ref );

			for (size_t j = 0; j < args; j++)
			{
				ScriptTypeVariant &scriptType = timer.args[j];

				switch (scriptType.m_type)
				{
				case LUA_TYPE_BOOLEAN:
					lua_pushboolean(pLuaState, (int)scriptType.m_boolean);
					break;

				case LUA_TYPE_INTEGER:
					lua_pushinteger(pLuaState, scriptType.m_integer);
					break;

				case LUA_TYPE_NUMBER:
					lua_pushnumber(pLuaState, scriptType.m_number);
					break;

				case LUA_TYPE_CSTRING:
					lua_pushstring(pLuaState, scriptType.m_cstring);
					break;

				case LUA_TYPE_CFUNCTION:
					lua_pushcfunction(pLuaState, scriptType.m_cfunction);
					break;

				//case LUA_TYPE_VECTOR:
				//	lua_newvector(pLuaState, &scriptType.m_vector);
				//	break;

				case LUA_TYPE_REFERENCE:
					lua_rawgeti(pLuaState, LUA_REGISTRYINDEX, (int)scriptType.m_ref);
					break;

				default:
					lua_pushnil(pLuaState);
					break;
				}
			}

			int luaResult = lua_pcall(pLuaState, args, 0, 0);

			luaL_unref( pLuaState, LUA_REGISTRYINDEX, (int)timer.func_ref );

			for (size_t j = 0; j < args; j++)
			{
				ScriptTypeVariant &scriptType = timer.args[j];

				switch (scriptType.m_type)
				{
				case LUA_TYPE_CSTRING:
					scriptType.Free();
					break;

				case LUA_TYPE_REFERENCE:
					luaL_unref( pLuaState, LUA_REGISTRYINDEX, (int)scriptType.m_ref );
					break;
				}
			}

			if ( luaResult != LUA_OK )
			{
				g_ScriptVM.PrintError();
			}

			m_vTimers.erase( m_vTimers.begin() + i );
			i--;
		}
	}
}

bool CTimersHandler::RemoveTimer(lua_Integer id)
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	for (size_t i = 0; i < m_vTimers.size(); i++)
	{
		if ( m_vTimers[i].id == id )
		{
			if ( pLuaState != NULL )
				luaL_unref( pLuaState, LUA_REGISTRYINDEX, (int)m_vTimers[i].func_ref );

			for (size_t j = 0; j < m_vTimers[i].args.size(); j++)
			{
				ScriptTypeVariant &scriptType = m_vTimers[i].args[j];

				switch (scriptType.m_type)
				{
				case LUA_TYPE_CSTRING:
					scriptType.Free();
					break;

				case LUA_TYPE_REFERENCE:
					if ( pLuaState != NULL )
						luaL_unref( pLuaState, LUA_REGISTRYINDEX, (int)scriptType.m_ref );
					break;
				}
			}

			m_vTimers.erase( m_vTimers.begin() + i );
			return true;
		}
	}

	return false;
}

void CTimersHandler::ClearTimers()
{
	lua_State *pLuaState = g_ScriptVM.GetVM();

	for (size_t i = 0; i < m_vTimers.size(); i++)
	{
		if ( pLuaState != NULL )
			luaL_unref( pLuaState, LUA_REGISTRYINDEX, (int)m_vTimers[i].func_ref );

		for (size_t j = 0; j < m_vTimers[i].args.size(); j++)
		{
			ScriptTypeVariant &scriptType = m_vTimers[i].args[j];

			switch (scriptType.m_type)
			{
			case LUA_TYPE_CSTRING:
				scriptType.Free();
				break;

			case LUA_TYPE_REFERENCE:
				if ( pLuaState != NULL )
					luaL_unref( pLuaState, LUA_REGISTRYINDEX, (int)scriptType.m_ref );
				break;
			}
		}
	}

	m_vTimers.clear();
}

//-----------------------------------------------------------------------------
// C to Lua
//-----------------------------------------------------------------------------

DEFINE_SCRIPTFUNC( CreateTimer )
{
	lua_Integer id = 0uLL;

	int args = lua_gettop(pLuaState);

	if ( args < 2 )
	{
		lua_pushstring( pLuaState, "CreateTimer: Expected 2 arguments as minimum [delay, function]");
		lua_error( pLuaState );

		lua_pushinteger(pLuaState, id);
		return VLUA_RET_ARGS( 1 );
	}

	double call_delay = (double)lua_tonumber(pLuaState, 1);

	if ( !lua_isfunction(pLuaState, 2) && !lua_iscfunction(pLuaState, 2) )
	{
		lua_pushstring( pLuaState, "CreateTimer: argument #2 expected as a function");
		lua_error( pLuaState );

		lua_pushinteger(pLuaState, id);
		return VLUA_RET_ARGS( 1 );
	}

	Lua_TimerContext t;

	g_TimersHandler.m_vTimers.push_back( t );
	Lua_TimerContext &timer = g_TimersHandler.m_vTimers.back();

	if ( g_TimersHandler.m_ID == 0uLL )
		g_TimersHandler.m_ID++;

	id = g_TimersHandler.m_ID++;
	double call_time = *dbRealtime + call_delay;

	lua_pushvalue(pLuaState, 2);

	scriptref_t func_ref = (scriptref_t)luaL_ref(pLuaState, LUA_REGISTRYINDEX);

	//Msg("luaL_ref(pLuaState, 2): %d\n", func_ref);

	timer.id = id;
	timer.time = call_time;
	timer.func_ref = func_ref;

	//Msg("args: %d\n", args);

	if ( args >= 3 )
	{
		for (int i = 3; i <= args; i++)
		{
			ScriptTypeVariant scriptType;

			if ( lua_isnoneornil(pLuaState, i) )
			{
				scriptType.m_type = LUA_TYPE_NIL;
			}
			else if ( lua_isboolean(pLuaState, i) )
			{
				scriptType = (bool)(!!lua_toboolean(pLuaState, i));
			}
			else if ( lua_isinteger(pLuaState, i) )
			{
				scriptType = (lua_Integer)lua_tointeger(pLuaState, i);
			}
			else if ( lua_isnumber(pLuaState, i) )
			{
				scriptType = (lua_Number)lua_tonumber(pLuaState, i);
			}
			else if ( lua_isstring(pLuaState, i) )
			{
				scriptType = (const char *)lua_tostring(pLuaState, i);
			}
			else if ( lua_iscfunction(pLuaState, i) )
			{
				scriptType = (lua_CFunction)lua_tocfunction(pLuaState, i);
			}
			//else if ( lua_isvector(pLuaState, i) )
			//{
			//	ScriptTypeVariant scriptType( *lua_getvector(pLuaState, i) );
			//}
			else
			{
				lua_pushvalue(pLuaState, i);

				scriptref_t ref = (scriptref_t)luaL_ref(pLuaState, LUA_REGISTRYINDEX);

				scriptType = ref;

				//Msg("scriptType[%d].m_ref (REFERENCE): %d\n", i, scriptType.m_ref);
			}

			//Msg("scriptType[%d].m_type: %d\n", i, scriptType.m_type);

			timer.args.push_back(scriptType);
		}
	}

	//luaL_unref(pLuaState, LUA_REGISTRYINDEX, func_ref);
	//g_TimersHandler.m_vTimers.pop_back();

	lua_pushinteger(pLuaState, id);
	return VLUA_RET_ARGS( 1 );
}

DEFINE_SCRIPTFUNC( RemoveTimer )
{
	lua_Integer id = lua_tointeger(pLuaState, 1);
	int removed = (int)g_TimersHandler.RemoveTimer( id );

	lua_pushboolean( pLuaState, removed );

	return VLUA_RET_ARGS( 1 );
}

DEFINE_SCRIPTFUNC( RemoveAllTimers )
{
	g_TimersHandler.ClearTimers();

	return VLUA_RET_ARGS( 0 );
}

//-----------------------------------------------------------------------------
// Init lib
//-----------------------------------------------------------------------------

LUALIB_API int luaopen_logic(lua_State *pLuaState)
{
	VLua::RegisterFunction( "CreateTimer", SCRIPTFUNC( CreateTimer ) );
	VLua::RegisterFunction( "RemoveTimer", SCRIPTFUNC( RemoveTimer ) );
	VLua::RegisterFunction( "RemoveAllTimers", SCRIPTFUNC( RemoveAllTimers ) );

	return 1;
}