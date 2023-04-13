#include <stdio.h>
#include <data_struct/hashdict.h>

#include "lua_entity_dictionary.h"
#include "lua_entity_vars.h"

#define EDICT_TYPE "edict"
//#define EDICT_NAME "edict"

//-----------------------------------------------------------------------------
// getters & setters binding
//-----------------------------------------------------------------------------

typedef int ( _cdecl *PropertyGetterFn )( lua_State * );
typedef int ( _cdecl *PropertySetterFn )( lua_State * );

static CHashDict<PropertyGetterFn, true, false> edict_getters( 7 );
static CHashDict<PropertySetterFn, true, false> edict_setters( 7 );

static int EdictInvokePropertyGetter( lua_State *pLuaState, const char *pszProperty )
{
	PropertyGetterFn *pfnPropertyGetter = edict_getters.Find( pszProperty );

	if ( !pfnPropertyGetter || !*pfnPropertyGetter )
		return 0;

	return ( *pfnPropertyGetter )( pLuaState );
}

static int EdictInvokePropertySetter( lua_State *pLuaState, const char *pszProperty )
{
	PropertySetterFn *pfnPropertySetter = edict_setters.Find( pszProperty );

	if ( !pfnPropertySetter || !*pfnPropertySetter )
		return 0;

	return ( *pfnPropertySetter )( pLuaState );
}

static void EdictBindProperty( const char *pszProperty, PropertyGetterFn getter, PropertySetterFn setter )
{
	if ( getter )
		edict_getters.Insert( pszProperty, getter );
	
	if ( setter )
		edict_setters.Insert( pszProperty, setter );
}

//-----------------------------------------------------------------------------
// Access functions
//-----------------------------------------------------------------------------

bool lua_isedict( lua_State *pLuaState, int i )
{
	if ( luaL_checkudata( pLuaState, i, EDICT_TYPE ) == NULL )
	{
		return false;
	}

	return true;
}

edict_t *lua_getedict( lua_State *pLuaState, int i )
{
	if ( luaL_checkudata( pLuaState, i, EDICT_TYPE ) == NULL )
	{
		luaL_typeerror( pLuaState, i, EDICT_TYPE );
	}

	return *(edict_t **)lua_touserdata( pLuaState, i );
}

void lua_pushedict( lua_State *pLuaState, edict_t *pEdict )
{
	// store pointer
	edict_t **e = (edict_t **)lua_newuserdata( pLuaState, sizeof( edict_t * ) );

	luaL_getmetatable( pLuaState, EDICT_TYPE );
	lua_setmetatable( pLuaState, -2 );

	*e = pEdict;
}

//-----------------------------------------------------------------------------
// Script functions
//-----------------------------------------------------------------------------

static int ScriptFunc_free_get( lua_State *pLuaState )
{
	edict_t *e = lua_getedict( pLuaState, 1 );
	
	lua_pushboolean( pLuaState, (int)e->free );

	return 1;
}

static int ScriptFunc_serialnumber_get( lua_State *pLuaState )
{
	edict_t *e = lua_getedict( pLuaState, 1 );
	
	lua_pushinteger( pLuaState, (int)e->serialnumber );

	return 1;
}

static int ScriptFunc_freetime_get( lua_State *pLuaState )
{
	edict_t *e = lua_getedict( pLuaState, 1 );
	
	lua_pushnumber( pLuaState, (lua_Number)e->freetime );

	return 1;
}

static int ScriptFunc_pvPrivateData_get( lua_State *pLuaState )
{
	edict_t *e = lua_getedict( pLuaState, 1 );

	lua_pushinteger( pLuaState, (int)e->pvPrivateData );

	return 1;
}

static int ScriptFunc_vars_get( lua_State *pLuaState )
{
	edict_t *e = lua_getedict( pLuaState, 1 );
	entvars_t *entityVars = &e->v;

	lua_pushentvars( pLuaState, entityVars );

	return 1;
}

static int ScriptFunc_MetaMethod_index( lua_State *pLuaState )
{
	const char *pszKey = luaL_checkstring( pLuaState, 2 );

	return EdictInvokePropertyGetter( pLuaState, pszKey );
}

static int ScriptFunc_MetaMethod_newindex( lua_State *pLuaState )
{
	const char *pszKey = luaL_checkstring( pLuaState, 2 );

	return EdictInvokePropertySetter( pLuaState, pszKey );
}

static int ScriptFunc_MetaMethod_tostring( lua_State *pLuaState )
{
	char s[ 64 ];
	edict_t *e = lua_getedict( pLuaState, 1 );

	snprintf( s, 64, "(%s : %X)", EDICT_TYPE, (unsigned long)e );

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

LUALIB_API int luaopen_edict( lua_State *pLuaState )
{
	luaL_newmetatable( pLuaState, EDICT_TYPE );
	luaL_setfuncs( pLuaState, Registrations, NULL );

	EdictBindProperty( "free", ScriptFunc_free_get, NULL );
	EdictBindProperty( "serialnumber", ScriptFunc_serialnumber_get, NULL );
	EdictBindProperty( "freetime", ScriptFunc_freetime_get, NULL );
	EdictBindProperty( "pvPrivateData", ScriptFunc_pvPrivateData_get, NULL );
	EdictBindProperty( "vars", ScriptFunc_vars_get, NULL );

	return 1;
}