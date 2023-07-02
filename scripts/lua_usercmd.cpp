#include <stdio.h>
#include <data_struct/hashdict.h>

#include "scripts_binding.h"
#include "lua_vector.h"
#include "lua_usercmd.h"

#include "../modules/server.h"

#define USERCMD_TYPE "usercmd"

//-----------------------------------------------------------------------------
// Access functions
//-----------------------------------------------------------------------------

bool lua_isusercmd( lua_State *pLuaState, int i )
{
	if ( luaL_checkudata( pLuaState, i, USERCMD_TYPE ) == NULL )
	{
		return false;
	}

	return true;
}

usercmd_t *lua_getusercmd( lua_State *pLuaState, int i )
{
	if ( luaL_checkudata( pLuaState, i, USERCMD_TYPE ) == NULL )
	{
		luaL_typeerror( pLuaState, i, USERCMD_TYPE );
	}

	return *(usercmd_t **)lua_touserdata( pLuaState, i );
}

void lua_pushusercmd( lua_State *pLuaState, usercmd_t *cmd )
{
	usercmd_t **usercmd = (usercmd_t **)lua_newuserdata( pLuaState, sizeof( usercmd_t * ) );

	luaL_getmetatable( pLuaState, USERCMD_TYPE );
	lua_setmetatable( pLuaState, -2 );

	*usercmd = cmd;
}

//-----------------------------------------------------------------------------
// Script functions
//-----------------------------------------------------------------------------

DEFINE_GETTER( USERCMD_TYPE, usercmd_t * );
DEFINE_SETTER( USERCMD_TYPE, usercmd_t * );

DEFINE_SCRIPTFUNC( MetaMethod_tostring )
{
	char s[ 64 ];
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );

	snprintf( s, 64, "(%s : %X)", USERCMD_TYPE, (unsigned long)cmd );

	lua_pushstring( pLuaState, s );

	return VLUA_RET_ARGS( 1 );
}

REG_BEGIN( Registrations )
	REG_GETTER()
	REG_SETTER()
	REG_SCRIPTFUNC( "__tostring", MetaMethod_tostring )
REG_END();

//-----------------------------------------------------------------------------
// Initialize
//-----------------------------------------------------------------------------

LUALIB_API int luaopen_usercmd( lua_State *pLuaState )
{
	VLua::RegisterMetaTable( USERCMD_TYPE );
	VLua::RegisterMetaTableFunctions( USERCMD_TYPE, Registrations );

	if ( !VLua::ArePropertiesInitialized() )
	{
		VLua::BindProperty( USERCMD_TYPE, "lerp_msec", VLuaPropertyDesc( usercmd_t, lerp_msec ) );
		VLua::BindProperty( USERCMD_TYPE, "msec", VLuaPropertyDescFieldtype( usercmd_t, msec, VLUA_FIELD_TYPE_BYTE ) );
		VLua::BindProperty( USERCMD_TYPE, "viewangles", VLuaPropertyDesc( usercmd_t, viewangles ) );
		VLua::BindProperty( USERCMD_TYPE, "forwardmove", VLuaPropertyDesc( usercmd_t, forwardmove ) );
		VLua::BindProperty( USERCMD_TYPE, "sidemove", VLuaPropertyDesc( usercmd_t, sidemove ) );
		VLua::BindProperty( USERCMD_TYPE, "upmove", VLuaPropertyDesc( usercmd_t, upmove ) );
		VLua::BindProperty( USERCMD_TYPE, "lightlevel", VLuaPropertyDesc( usercmd_t, lightlevel ) );
		VLua::BindProperty( USERCMD_TYPE, "buttons", VLuaPropertyDesc( usercmd_t, buttons ) );
		VLua::BindProperty( USERCMD_TYPE, "impulse", VLuaPropertyDescFieldtype( usercmd_t, impulse, VLUA_FIELD_TYPE_BYTE ) );
		VLua::BindProperty( USERCMD_TYPE, "weaponselect", VLuaPropertyDescFieldtype( usercmd_t, weaponselect, VLUA_FIELD_TYPE_BYTE ) );
		VLua::BindProperty( USERCMD_TYPE, "impact_index", VLuaPropertyDesc( usercmd_t, impact_index ) );
		VLua::BindProperty( USERCMD_TYPE, "impact_position", VLuaPropertyDesc( usercmd_t, impact_position ) );
	}

	return 1;
}