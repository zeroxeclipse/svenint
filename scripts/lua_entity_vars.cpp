#include <stdio.h>
#include <data_struct/hashdict.h>

#include "lua_vector.h"
#include "lua_entity_vars.h"

#include "../modules/server.h"

#define ENTVARS_TYPE "entvars"
//#define ENTVARS_NAME "entvars"

//-----------------------------------------------------------------------------
// getters & setters binding
//-----------------------------------------------------------------------------

typedef int ( _cdecl *PropertyGetterFn )( lua_State * );
typedef int ( _cdecl *PropertySetterFn )( lua_State * );

static CHashDict<PropertyGetterFn, true, false> edict_getters( 7 );
static CHashDict<PropertySetterFn, true, false> edict_setters( 7 );

static int EntvarsInvokePropertyGetter( lua_State *pLuaState, const char *pszProperty )
{
	PropertyGetterFn *pfnPropertyGetter = edict_getters.Find( pszProperty );

	if ( !pfnPropertyGetter || !*pfnPropertyGetter )
		return 0;

	return ( *pfnPropertyGetter )( pLuaState );
}

static int EntvarsInvokePropertySetter( lua_State *pLuaState, const char *pszProperty )
{
	PropertySetterFn *pfnPropertySetter = edict_setters.Find( pszProperty );

	if ( !pfnPropertySetter || !*pfnPropertySetter )
		return 0;

	return ( *pfnPropertySetter )( pLuaState );
}

static void EntvarsBindProperty( const char *pszProperty, PropertyGetterFn getter, PropertySetterFn setter )
{
	if ( getter )
		edict_getters.Insert( pszProperty, getter );
	
	if ( setter )
		edict_setters.Insert( pszProperty, setter );
}

//-----------------------------------------------------------------------------
// Access functions
//-----------------------------------------------------------------------------

bool lua_isentvars( lua_State *pLuaState, int i )
{
	if ( luaL_checkudata( pLuaState, i, ENTVARS_TYPE ) == NULL )
	{
		return false;
	}

	return true;
}

entvars_t *lua_getentvars( lua_State *pLuaState, int i )
{
	if ( luaL_checkudata( pLuaState, i, ENTVARS_TYPE ) == NULL )
	{
		luaL_typeerror( pLuaState, i, ENTVARS_TYPE );
	}

	return *(entvars_t **)lua_touserdata( pLuaState, i );
}

void lua_pushentvars( lua_State *pLuaState, entvars_t *pEntityVars )
{
	entvars_t **vars = (entvars_t **)lua_newuserdata( pLuaState, sizeof( entvars_t * ) );

	luaL_getmetatable( pLuaState, ENTVARS_TYPE );
	lua_setmetatable( pLuaState, -2 );

	*vars = pEntityVars;
}

//-----------------------------------------------------------------------------
// Script functions
//-----------------------------------------------------------------------------

static int ScriptFunc_origin_get( lua_State *pLuaState )
{
	entvars_t *entvars = lua_getentvars( pLuaState, 1 );
	
	lua_newvector( pLuaState, &entvars->origin );

	return 1;
}

static int ScriptFunc_origin_set( lua_State *pLuaState )
{
	entvars_t *entvars = lua_getentvars( pLuaState, 1 );
	Vector *v = lua_getvector( pLuaState, 3 );
	
	entvars->origin.x = v->x;
	entvars->origin.y = v->y;
	entvars->origin.z = v->z;

	lua_newvector( pLuaState, v );

	return 1;
}

static int ScriptFunc_netname_get( lua_State *pLuaState )
{
	entvars_t *entvars = lua_getentvars( pLuaState, 1 );
	const char *pszNetname = ( entvars->netname ? gpGlobals->pStringBase + entvars->netname : NULL );

	lua_pushstring( pLuaState, pszNetname );

	return 1;
}

static int ScriptFunc_MetaMethod_index( lua_State *pLuaState )
{
	const char *pszKey = luaL_checkstring( pLuaState, 2 );

	return EntvarsInvokePropertyGetter( pLuaState, pszKey );
}

static int ScriptFunc_MetaMethod_newindex( lua_State *pLuaState )
{
	const char *pszKey = luaL_checkstring( pLuaState, 2 );

	return EntvarsInvokePropertySetter( pLuaState, pszKey );
}

static int ScriptFunc_MetaMethod_tostring( lua_State *pLuaState )
{
	char s[ 64 ];
	entvars_t *entvars = lua_getentvars( pLuaState, 1 );

	snprintf( s, 64, "(%s : %X)", ENTVARS_TYPE, (unsigned long)entvars );

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

LUALIB_API int luaopen_entvars( lua_State *pLuaState )
{
	luaL_newmetatable( pLuaState, ENTVARS_TYPE );
	luaL_setfuncs( pLuaState, Registrations, NULL );

	EntvarsBindProperty( "origin", ScriptFunc_origin_get, ScriptFunc_origin_set );
	EntvarsBindProperty( "netname", ScriptFunc_netname_get, NULL );

	return 1;
}