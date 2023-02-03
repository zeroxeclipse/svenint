#include "lua_debug.h"

#include <dbg.h>

static const Color clr_print(80, 186, 255, 255);
static const Color clr_error_print(255, 90, 90, 255);

//-----------------------------------------------------------------------------
// Printing
//-----------------------------------------------------------------------------

static int print(lua_State *pLuaState)
{
	//switch ( lua_type(pLuaState, 1) )
	//{
	//case LUA_TNUMBER:
	//	ConColorMsg( clr_print, "%g", lua_tonumber(pLuaState, 1) );
	//	break;

	//case LUA_TSTRING:
	//	ConColorMsg( clr_print, lua_tostring(pLuaState, 1) );
	//	break;

	//case LUA_TBOOLEAN:
	//	ConColorMsg( clr_print, "%s", (lua_toboolean(pLuaState, 1) ? "true" : "false") );
	//	break;

	//case LUA_TNIL:
	//	ConColorMsg( clr_print, "(nil)" );
	//	break;

	//default:
	//	ConColorMsg( clr_print, "(%p)", lua_topointer(pLuaState, 1) );
	//	break;
	//}

	ConColorMsg( clr_print, luaL_tolstring(pLuaState, -1, NULL) );

	return 0;
}

static int printl(lua_State *pLuaState)
{
	//try
	//{
	//	switch ( lua_type(pLuaState, 1) )
	//	{
	//	case LUA_TNUMBER:
	//		ConColorMsg( clr_print, "%g\n", lua_tonumber(pLuaState, 1) );
	//		break;

	//	case LUA_TSTRING:
	//		ConColorMsg( clr_print, "%s\n", lua_tostring(pLuaState, 1));
	//		break;

	//	case LUA_TBOOLEAN:
	//		ConColorMsg( clr_print, "%s\n", (lua_toboolean(pLuaState, 1) ? "true" : "false") );
	//		break;

	//	case LUA_TNIL:
	//		ConColorMsg( clr_print, "nil\n" );
	//		break;
	//	
	//	case LUA_TTABLE:
	//		ConColorMsg( clr_print, "table: %p\n", lua_topointer(pLuaState, 1));
	//		break;
	//	
	//	case LUA_TFUNCTION:
	//		ConColorMsg( clr_print, "function: %p\n", lua_topointer(pLuaState, 1));
	//		break;
	//	
	//	case LUA_TTHREAD:
	//		ConColorMsg( clr_print, "thread: %p\n", lua_topointer(pLuaState, 1));
	//		break;
	//	
	//	case LUA_TLIGHTUSERDATA:
	//	{
	//		ConColorMsg( clr_print, "nil\n" );
	//		break;
	//	}
	//	
	//	case LUA_TUSERDATA:
	//		ConColorMsg( clr_print, "nil\n" );
	//		break;

	//	default:
	//		ConColorMsg( clr_print, "unknown: %p\n", lua_topointer(pLuaState, 1) );
	//		break;
	//	}
	//}
	//catch (const char *pszString)
	//{
	//	Warning("printl: Unable to convert userdata to string\n%s\n", pszString);
	//}

	ConColorMsg( clr_print, "%s\n", luaL_tolstring(pLuaState, -1, NULL) );

	return 0;
}

static int printerror(lua_State *pLuaState)
{
	ConColorMsg( clr_error_print, luaL_tolstring(pLuaState, -1, NULL) );

	return 0;
}

static int printerrorl(lua_State *pLuaState)
{
	ConColorMsg( clr_error_print, "%s\n", luaL_tolstring(pLuaState, -1, NULL) );

	return 0;
}

LUALIB_API int luaopen_print(lua_State *pLuaState)
{
	lua_pushcfunction(pLuaState, print);
	lua_setglobal(pLuaState, "print");

	lua_pushcfunction(pLuaState, printl);
	lua_setglobal(pLuaState, "printl");
	
	lua_pushcfunction(pLuaState, printerror);
	lua_setglobal(pLuaState, "printerror");

	lua_pushcfunction(pLuaState, printerrorl);
	lua_setglobal(pLuaState, "printerrorl");

	return 1;
}