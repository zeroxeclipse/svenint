#include "lua_cvar.h"

#include <ICvar.h>

static int CVar_FindCvar(lua_State *pLuaState)
{
	const char *pszCvarName = lua_tostring(pLuaState, 1);

	lua_pushboolean( pLuaState, CVar()->FindCvar(pszCvarName) != NULL );

	return 1;
}

static int CVar_FindCmd(lua_State *pLuaState)
{
	const char *pszCmdName = lua_tostring(pLuaState, 1);

	lua_pushboolean( pLuaState, CVar()->FindCmd(pszCmdName) != NULL );

	return 1;
}

static int CVar_SetValue(lua_State *pLuaState)
{
	const char *pszCvarName = lua_tostring(pLuaState, 1);
	cvar_t *pCvar = CVar()->FindCvar(pszCvarName);

	if ( pCvar == NULL )
	{
		lua_pushboolean(pLuaState, false);
		return 1;
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

	return 1;
}

static int CVar_GetFloat(lua_State *pLuaState)
{
	const char *pszCvarName = lua_tostring(pLuaState, 1);
	cvar_t *pCvar = CVar()->FindCvar(pszCvarName);

	if ( pCvar == NULL )
	{
		lua_pushnil(pLuaState);
		return 1;
	}

	lua_Number dbValue = (lua_Number)CVar()->GetFloatFromCvar(pCvar);
	lua_pushnumber(pLuaState, dbValue);

	return 1;
}

static int CVar_GetInteger(lua_State *pLuaState)
{
	const char *pszCvarName = lua_tostring(pLuaState, 1);
	cvar_t *pCvar = CVar()->FindCvar(pszCvarName);

	if ( pCvar == NULL )
	{
		lua_pushnil(pLuaState);
		return 1;
	}

	lua_Integer iValue = (lua_Integer)CVar()->GetIntFromCvar(pCvar);
	lua_pushinteger(pLuaState, iValue);

	return 1;
}

static int CVar_GetString(lua_State *pLuaState)
{
	const char *pszCvarName = lua_tostring(pLuaState, 1);
	cvar_t *pCvar = CVar()->FindCvar(pszCvarName);

	if ( pCvar == NULL )
	{
		lua_pushnil(pLuaState);
		return 1;
	}

	const char *pszValue = CVar()->GetStringFromCvar(pCvar);
	lua_pushstring(pLuaState, pszValue);

	return 1;
}

static int CVar_GetBool(lua_State *pLuaState)
{
	const char *pszCvarName = lua_tostring(pLuaState, 1);
	cvar_t *pCvar = CVar()->FindCvar(pszCvarName);

	if ( pCvar == NULL )
	{
		lua_pushnil(pLuaState);
		return 1;
	}

	bool bValue = CVar()->GetBoolFromCvar(pCvar);
	lua_pushboolean(pLuaState, bValue);

	return 1;
}

static const luaL_Reg CVar_Registrations[] =
{
	{ "FindCvar",	CVar_FindCvar },
	{ "FindCmd",	CVar_FindCmd },
	{ "SetValue",	CVar_SetValue },
	{ "GetFloat",	CVar_GetFloat },
	{ "GetInteger",	CVar_GetInteger },
	{ "GetString",	CVar_GetString },
	{ "GetBool",	CVar_GetBool },
	{ NULL,			NULL }
};

LUALIB_API int luaopen_cvar(lua_State *pLuaState)
{
	lua_newtable(pLuaState);
	luaL_setfuncs(pLuaState, CVar_Registrations, NULL);
	lua_setglobal(pLuaState, "CVar");

	return 1;
}