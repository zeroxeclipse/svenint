#include <stdio.h>
#include <data_struct/hashdict.h>

#include "lua_vector.h"
#include "lua_usercmd.h"

#include "../modules/server.h"

#define USERCMD_TYPE "usercmd"

//-----------------------------------------------------------------------------
// getters & setters binding
//-----------------------------------------------------------------------------

typedef int ( _cdecl *PropertyGetterFn )( lua_State * );
typedef int ( _cdecl *PropertySetterFn )( lua_State * );

static CHashDict<PropertyGetterFn, true, false> usercmd_getters( 7 );
static CHashDict<PropertySetterFn, true, false> usercmd_setters( 7 );

static int UsercmdInvokePropertyGetter( lua_State *pLuaState, const char *pszProperty )
{
	PropertyGetterFn *pfnPropertyGetter = usercmd_getters.Find( pszProperty );

	if ( !pfnPropertyGetter || !*pfnPropertyGetter )
		return 0;

	return ( *pfnPropertyGetter )( pLuaState );
}

static int UsercmdInvokePropertySetter( lua_State *pLuaState, const char *pszProperty )
{
	PropertySetterFn *pfnPropertySetter = usercmd_setters.Find( pszProperty );

	if ( !pfnPropertySetter || !*pfnPropertySetter )
		return 0;

	return ( *pfnPropertySetter )( pLuaState );
}

static void UsercmdBindProperty( const char *pszProperty, PropertyGetterFn getter, PropertySetterFn setter )
{
	if ( getter )
		usercmd_getters.Insert( pszProperty, getter );
	
	if ( setter )
		usercmd_setters.Insert( pszProperty, setter );
}

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

// lerp_msec

static int ScriptFunc_lerp_msec_get( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );
	
	lua_pushinteger( pLuaState, (lua_Integer)cmd->lerp_msec );

	return 1;
}

static int ScriptFunc_lerp_msec_set( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );
	short lerp_msec = (short)( lua_tointeger( pLuaState, 3 ) & 0xFFFF );
	
	cmd->lerp_msec = lerp_msec;

	lua_pushinteger( pLuaState, (lua_Integer)lerp_msec );

	return 1;
}

// msec

static int ScriptFunc_msec_get( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );
	
	lua_pushinteger( pLuaState, (lua_Integer)cmd->msec );

	return 1;
}

static int ScriptFunc_msec_set( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );
	byte msec = (byte)( lua_tointeger( pLuaState, 3 ) & 0xFF );
	
	cmd->msec = msec;

	lua_pushinteger( pLuaState, (lua_Integer)msec );

	return 1;
}

// viewangles

static int ScriptFunc_viewangles_get( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );

	lua_newvector( pLuaState, &cmd->viewangles );

	return 1;
}

static int ScriptFunc_viewangles_set( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );
	
	Vector *v = lua_getvector( pLuaState, 3 );

	cmd->viewangles.x = v->x;
	cmd->viewangles.y = v->y;
	cmd->viewangles.z = v->z;

	lua_newvector( pLuaState, v );

	return 1;
}

// forwardmove

static int ScriptFunc_forwardmove_get( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );

	lua_pushnumber( pLuaState, (lua_Number)cmd->forwardmove );

	return 1;
}

static int ScriptFunc_forwardmove_set( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );
	float forwardmove = (float)lua_tonumber( pLuaState, 3 );

	cmd->forwardmove = forwardmove;

	lua_pushnumber( pLuaState, (lua_Number)forwardmove );

	return 1;
}

// sidemove

static int ScriptFunc_sidemove_get( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );

	lua_pushnumber( pLuaState, (lua_Number)cmd->sidemove );

	return 1;
}

static int ScriptFunc_sidemove_set( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );
	float sidemove = (float)lua_tonumber( pLuaState, 3 );

	cmd->sidemove = sidemove;

	lua_pushnumber( pLuaState, (lua_Number)sidemove );

	return 1;
}

// upmove

static int ScriptFunc_upmove_get( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );

	lua_pushnumber( pLuaState, (lua_Number)cmd->upmove );

	return 1;
}

static int ScriptFunc_upmove_set( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );
	float upmove = (float)lua_tonumber( pLuaState, 3 );

	cmd->upmove = upmove;

	lua_pushnumber( pLuaState, (lua_Number)upmove );

	return 1;
}

// lightlevel

static int ScriptFunc_lightlevel_get( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );

	lua_pushinteger( pLuaState, (lua_Integer)cmd->lightlevel );

	return 1;
}

static int ScriptFunc_lightlevel_set( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );
	byte lightlevel = (byte)( lua_tointeger( pLuaState, 3 ) & 0xFF );

	cmd->lightlevel = lightlevel;

	lua_pushinteger( pLuaState, (lua_Integer)lightlevel );

	return 1;
}

// buttons

static int ScriptFunc_buttons_get( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );

	lua_pushinteger( pLuaState, (lua_Integer)cmd->buttons );

	return 1;
}

static int ScriptFunc_buttons_set( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );
	unsigned short buttons = (unsigned short)( lua_tointeger( pLuaState, 3 ) & 0xFFFF );

	cmd->buttons = buttons;

	lua_pushinteger( pLuaState, (lua_Integer)buttons );

	return 1;
}

// impulse

static int ScriptFunc_impulse_get( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );

	lua_pushinteger( pLuaState, (lua_Integer)cmd->impulse );

	return 1;
}

static int ScriptFunc_impulse_set( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );
	byte impulse = (byte)( lua_tointeger( pLuaState, 3 ) & 0xFF );

	cmd->impulse = impulse;

	lua_pushinteger( pLuaState, (lua_Integer)impulse );

	return 1;
}

// weaponselect

static int ScriptFunc_weaponselect_get( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );

	lua_pushinteger( pLuaState, (lua_Integer)cmd->weaponselect );

	return 1;
}

static int ScriptFunc_weaponselect_set( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );
	byte weaponselect = (byte)( lua_tointeger( pLuaState, 3 ) & 0xFF );

	cmd->weaponselect = weaponselect;

	lua_pushinteger( pLuaState, (lua_Integer)weaponselect );

	return 1;
}

// impact_index

static int ScriptFunc_impact_index_get( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );

	lua_pushinteger( pLuaState, (lua_Integer)cmd->impact_index );

	return 1;
}

static int ScriptFunc_impact_index_set( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );
	int impact_index = (int)( lua_tointeger( pLuaState, 3 ) & 0xFFFFFFFF );

	cmd->impact_index = impact_index;

	lua_pushinteger( pLuaState, (lua_Integer)impact_index );

	return 1;
}

// impact_position

static int ScriptFunc_impact_position_get( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );

	lua_newvector( pLuaState, &cmd->impact_position );

	return 1;
}

static int ScriptFunc_impact_position_set( lua_State *pLuaState )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );

	Vector *v = lua_getvector( pLuaState, 3 );

	cmd->impact_position.x = v->x;
	cmd->impact_position.y = v->y;
	cmd->impact_position.z = v->z;

	lua_newvector( pLuaState, v );

	return 1;
}

//-----------------------------------------------------------------------------
// Guts
//-----------------------------------------------------------------------------

static int ScriptFunc_MetaMethod_index( lua_State *pLuaState )
{
	const char *pszKey = luaL_checkstring( pLuaState, 2 );

	return UsercmdInvokePropertyGetter( pLuaState, pszKey );
}

static int ScriptFunc_MetaMethod_newindex( lua_State *pLuaState )
{
	const char *pszKey = luaL_checkstring( pLuaState, 2 );

	return UsercmdInvokePropertySetter( pLuaState, pszKey );
}

static int ScriptFunc_MetaMethod_tostring( lua_State *pLuaState )
{
	char s[ 64 ];
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );

	snprintf( s, 64, "(%s : %X)", USERCMD_TYPE, (unsigned long)cmd );

	lua_pushstring( pLuaState, s );

	return 1;
}

static const luaL_Reg Registrations[] =
{
	{ "__index",	ScriptFunc_MetaMethod_index },
	{ "__newindex",	ScriptFunc_MetaMethod_newindex	},
	{ "__tostring",	ScriptFunc_MetaMethod_tostring	},
	{ NULL,			NULL }
};

//-----------------------------------------------------------------------------
// Initialize
//-----------------------------------------------------------------------------

LUALIB_API int luaopen_usercmd( lua_State *pLuaState )
{
	luaL_newmetatable( pLuaState, USERCMD_TYPE );
	luaL_setfuncs( pLuaState, Registrations, NULL );

	UsercmdBindProperty( "lerp_msec", ScriptFunc_lerp_msec_get, ScriptFunc_lerp_msec_set );
	UsercmdBindProperty( "msec", ScriptFunc_msec_get, ScriptFunc_msec_set );
	UsercmdBindProperty( "viewangles", ScriptFunc_viewangles_get, ScriptFunc_viewangles_set );
	UsercmdBindProperty( "forwardmove", ScriptFunc_forwardmove_get, ScriptFunc_forwardmove_set );
	UsercmdBindProperty( "sidemove", ScriptFunc_sidemove_get, ScriptFunc_sidemove_set );
	UsercmdBindProperty( "upmove", ScriptFunc_upmove_get, ScriptFunc_upmove_set );
	UsercmdBindProperty( "lightlevel", ScriptFunc_lightlevel_get, ScriptFunc_lightlevel_set );
	UsercmdBindProperty( "buttons", ScriptFunc_buttons_get, ScriptFunc_buttons_set );
	UsercmdBindProperty( "impulse", ScriptFunc_impulse_get, ScriptFunc_impulse_set );
	UsercmdBindProperty( "weaponselect", ScriptFunc_weaponselect_get, ScriptFunc_weaponselect_set );
	UsercmdBindProperty( "impact_index", ScriptFunc_impact_index_get, ScriptFunc_impact_index_set );
	UsercmdBindProperty( "impact_position", ScriptFunc_impact_position_get, ScriptFunc_impact_position_set );

	return 1;
}