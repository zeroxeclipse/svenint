#include "lua_debug.h"
#include "scripts.h"
#include "scripts_binding.h"

#include <dbg.h>

static const Color clr_print(80, 186, 255, 255);
static const Color clr_error_print(255, 90, 90, 255);

//-----------------------------------------------------------------------------
// Printing
//-----------------------------------------------------------------------------

DEFINE_SCRIPTFUNC( print )
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

	return VLUA_RET_ARGS( 0 );
}

DEFINE_SCRIPTFUNC( printl )
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

	return VLUA_RET_ARGS( 0 );
}

DEFINE_SCRIPTFUNC( printerror )
{
	ConColorMsg( clr_error_print, luaL_tolstring(pLuaState, -1, NULL) );

	return VLUA_RET_ARGS( 0 );
}

DEFINE_SCRIPTFUNC( printerrorl )
{
	ConColorMsg( clr_error_print, "%s\n", luaL_tolstring(pLuaState, -1, NULL) );

	return VLUA_RET_ARGS( 0 );
}

LUALIB_API int luaopen_print(lua_State *pLuaState)
{
	VLua::RegisterFunction( "print", SCRIPTFUNC( print ) );
	VLua::RegisterFunction( "printl", SCRIPTFUNC( printl ) );
	VLua::RegisterFunction( "printerror", SCRIPTFUNC( printerror ) );
	VLua::RegisterFunction( "printerrorl", SCRIPTFUNC( printerrorl ) );

	// printl wrapper for string formatting
	g_ScriptVM.RunScript( "function printf( format, ... ) printl( string.format( format, ... ) ) end");

	return 1;
}