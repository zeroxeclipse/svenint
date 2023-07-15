#include "scripts_binding.h"
#include "lua_random.h"

#include "../utils/random.h"

DEFINE_SCRIPTFUNC( RandomSetSeed )
{
	int seed = (int)lua_tointeger( pLuaState, 1 );

	random.SetSeed( seed );

	return VLUA_RET_ARGS( 0 );
}

DEFINE_SCRIPTFUNC( RandomInt )
{
	int low = (int)lua_tointeger( pLuaState, 1 );
	int high = (int)lua_tointeger( pLuaState, 2 );

	int val = random.RandomInt( low, high );

	lua_pushinteger( pLuaState, (lua_Integer)val );

	return VLUA_RET_ARGS( 1 );
}

DEFINE_SCRIPTFUNC( RandomFloat )
{
	float low = (float)lua_tonumber( pLuaState, 1 );
	float high = (float)lua_tonumber( pLuaState, 2 );

	float val = random.RandomFloat( low, high );

	lua_pushnumber( pLuaState, (lua_Number)val );

	return VLUA_RET_ARGS( 1 );
}

DEFINE_SCRIPTFUNC( RandomFloatExp )
{
	float low = (float)lua_tonumber( pLuaState, 1 );
	float high = (float)lua_tonumber( pLuaState, 2 );
	float exp = (float)lua_tonumber( pLuaState, 3 );

	float val = random.RandomFloatExp( low, high, exp );

	lua_pushnumber( pLuaState, (lua_Number)val );

	return VLUA_RET_ARGS( 1 );
}

LUALIB_API int luaopen_random(lua_State *pLuaState)
{
	VLua::RegisterFunction( "RandomSetSeed", SCRIPTFUNC( RandomSetSeed ) );
	VLua::RegisterFunction( "RandomInt", SCRIPTFUNC( RandomInt ) );
	VLua::RegisterFunction( "RandomFloat", SCRIPTFUNC( RandomFloat ) );
	VLua::RegisterFunction( "RandomFloatExp", SCRIPTFUNC( RandomFloatExp ) );

	return 1;
}