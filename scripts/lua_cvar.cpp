#include "scripts_binding.h"
#include "lua_cvar.h"

#include <ICvar.h>

DEFINE_SCRIPTFUNC( FindCvar )
{
	const char *pszCvarName = lua_tostring(pLuaState, 1);

	lua_pushboolean( pLuaState, CVar()->FindCvar(pszCvarName) != NULL );

	return VLUA_RET_ARGS( 1 );
}

DEFINE_SCRIPTFUNC( FindCmd )
{
	const char *pszCmdName = lua_tostring(pLuaState, 1);

	lua_pushboolean( pLuaState, CVar()->FindCmd(pszCmdName) != NULL );

	return VLUA_RET_ARGS( 1 );
}

DEFINE_SCRIPTFUNC( SetValue )
{
	const char *pszCvarName = lua_tostring(pLuaState, 1);
	cvar_t *pCvar = CVar()->FindCvar(pszCvarName);

	if ( pCvar == NULL )
	{
		lua_pushboolean(pLuaState, false);
		return VLUA_RET_ARGS( 1 );
	}

	if ( lua_isinteger(pLuaState, 2) )
	{
		int value = (int)lua_tointeger(pLuaState, 2);
		CVar()->SetValue(pCvar, value);

		lua_pushboolean(pLuaState, true);
	}
	else if ( lua_isnumber(pLuaState, 2) )
	{
		float value = static_cast<float>(lua_tonumber(pLuaState, 2));
		CVar()->SetValue(pCvar, value);

		lua_pushboolean(pLuaState, true);
	}
	else if ( lua_isstring(pLuaState, 2) )
	{
		const char *value = lua_tostring(pLuaState, 2);
		CVar()->SetValue(pCvar, value);

		lua_pushboolean(pLuaState, true);
	}
	else if ( lua_isboolean(pLuaState, 2) )
	{
		bool value = !!lua_toboolean(pLuaState, 2);
		CVar()->SetValue(pCvar, value);

		lua_pushboolean(pLuaState, true);
	}
	else
	{
		lua_pushboolean(pLuaState, false);
	}

	return VLUA_RET_ARGS( 1 );
}

DEFINE_SCRIPTFUNC( GetFloat )
{
	const char *pszCvarName = lua_tostring(pLuaState, 1);
	cvar_t *pCvar = CVar()->FindCvar(pszCvarName);

	if ( pCvar == NULL )
	{
		lua_pushnil(pLuaState);
		return VLUA_RET_ARGS( 1 );
	}

	lua_Number dbValue = (lua_Number)CVar()->GetFloatFromCvar(pCvar);
	lua_pushnumber(pLuaState, dbValue);

	return VLUA_RET_ARGS( 1 );
}

DEFINE_SCRIPTFUNC( GetInteger )
{
	const char *pszCvarName = lua_tostring(pLuaState, 1);
	cvar_t *pCvar = CVar()->FindCvar(pszCvarName);

	if ( pCvar == NULL )
	{
		lua_pushnil(pLuaState);
		return VLUA_RET_ARGS( 1 );
	}

	lua_Integer iValue = (lua_Integer)CVar()->GetIntFromCvar(pCvar);
	lua_pushinteger(pLuaState, iValue);

	return VLUA_RET_ARGS( 1 );
}

DEFINE_SCRIPTFUNC( GetString )
{
	const char *pszCvarName = lua_tostring(pLuaState, 1);
	cvar_t *pCvar = CVar()->FindCvar(pszCvarName);

	if ( pCvar == NULL )
	{
		lua_pushnil(pLuaState);
		return VLUA_RET_ARGS( 1 );
	}

	const char *pszValue = CVar()->GetStringFromCvar(pCvar);
	lua_pushstring(pLuaState, pszValue);

	return VLUA_RET_ARGS( 1 );
}

DEFINE_SCRIPTFUNC( GetBool )
{
	const char *pszCvarName = lua_tostring(pLuaState, 1);
	cvar_t *pCvar = CVar()->FindCvar(pszCvarName);

	if ( pCvar == NULL )
	{
		lua_pushnil(pLuaState);
		return VLUA_RET_ARGS( 1 );
	}

	bool bValue = CVar()->GetBoolFromCvar(pCvar);
	lua_pushboolean(pLuaState, bValue);

	return VLUA_RET_ARGS( 1 );
}

REG_BEGIN( Registrations )
	REG_SCRIPTFUNC( "FindCvar", FindCvar )
	REG_SCRIPTFUNC( "FindCmd", FindCmd )
	REG_SCRIPTFUNC( "SetValue", SetValue )
	REG_SCRIPTFUNC( "GetFloat", GetFloat )
	REG_SCRIPTFUNC( "GetInteger", GetInteger )
	REG_SCRIPTFUNC( "GetString", GetString )
	REG_SCRIPTFUNC( "GetBool", GetBool )
REG_END();

LUALIB_API int luaopen_cvar(lua_State *pLuaState)
{
	VLua::RegisterTable( "CVar" );
	VLua::RegisterTableFunctions( "CVar", Registrations );

	return 1;
}