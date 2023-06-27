#include <stdio.h>
#include <data_struct/hashdict.h>
#include <hl_sdk/pm_shared/pm_shared.h>

#include "lua_player_move.h"

#define PLAYERMOVE_TYPE "playermove"
//#define EDICT_NAME "edict"

//-----------------------------------------------------------------------------
// getters & setters binding
//-----------------------------------------------------------------------------

typedef int ( _cdecl *PropertyGetterFn )( lua_State * );
typedef int ( _cdecl *PropertySetterFn )( lua_State * );

static CHashDict<PropertyGetterFn, true, false> edict_getters( 7 );
static CHashDict<PropertySetterFn, true, false> edict_setters( 7 );

static int PlayerMoveInvokePropertyGetter( lua_State *pLuaState, const char *pszProperty )
{
	PropertyGetterFn *pfnPropertyGetter = edict_getters.Find( pszProperty );

	if ( !pfnPropertyGetter || !*pfnPropertyGetter )
		return 0;

	return ( *pfnPropertyGetter )( pLuaState );
}

static int PlayerMoveInvokePropertySetter( lua_State *pLuaState, const char *pszProperty )
{
	PropertySetterFn *pfnPropertySetter = edict_setters.Find( pszProperty );

	if ( !pfnPropertySetter || !*pfnPropertySetter )
		return 0;

	return ( *pfnPropertySetter )( pLuaState );
}

static void PlayerMoveBindProperty( const char *pszProperty, PropertyGetterFn getter, PropertySetterFn setter )
{
	if ( getter )
		edict_getters.Insert( pszProperty, getter );
	
	if ( setter )
		edict_setters.Insert( pszProperty, setter );
}

//-----------------------------------------------------------------------------
// Access functions
//-----------------------------------------------------------------------------

bool lua_isplayermove( lua_State *pLuaState, int i )
{
	if ( luaL_checkudata( pLuaState, i, PLAYERMOVE_TYPE ) == NULL )
	{
		return false;
	}

	return true;
}

playermove_t *lua_getplayermove( lua_State *pLuaState, int i )
{
	if ( luaL_checkudata( pLuaState, i, PLAYERMOVE_TYPE ) == NULL )
	{
		luaL_typeerror( pLuaState, i, PLAYERMOVE_TYPE );
	}

	return *(playermove_t **)lua_touserdata( pLuaState, i );
}

void lua_pushplayermove( lua_State *pLuaState, playermove_t *pPlayerMove )
{
	// store pointer
	playermove_t **pm = (playermove_t **)lua_newuserdata( pLuaState, sizeof( playermove_t * ) );

	luaL_getmetatable( pLuaState, PLAYERMOVE_TYPE );
	lua_setmetatable( pLuaState, -2 );

	*pm = pPlayerMove;
}

//-----------------------------------------------------------------------------
// Script functions
//-----------------------------------------------------------------------------

static int ScriptFunc_flags_get( lua_State *pLuaState )
{
	playermove_t *pm = lua_getplayermove( pLuaState, 1 );

	lua_pushinteger( pLuaState, (int)pm->flags );

	return 1;
}

static int ScriptFunc_MetaMethod_index( lua_State *pLuaState )
{
	const char *pszKey = luaL_checkstring( pLuaState, 2 );

	return PlayerMoveInvokePropertyGetter( pLuaState, pszKey );
}

static int ScriptFunc_MetaMethod_newindex( lua_State *pLuaState )
{
	const char *pszKey = luaL_checkstring( pLuaState, 2 );

	return PlayerMoveInvokePropertySetter( pLuaState, pszKey );
}

static int ScriptFunc_MetaMethod_tostring( lua_State *pLuaState )
{
	char s[ 64 ];
	playermove_t *pm = lua_getplayermove( pLuaState, 1 );

	snprintf( s, 64, "(%s : %X)", PLAYERMOVE_TYPE, (unsigned long)pm );

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

LUALIB_API int luaopen_playermove( lua_State *pLuaState )
{
	luaL_newmetatable( pLuaState, PLAYERMOVE_TYPE );
	luaL_setfuncs( pLuaState, Registrations, NULL );

	PlayerMoveBindProperty( "flags", ScriptFunc_flags_get, NULL );

	lua_pushplayermove( pLuaState, g_pPlayerMove );
	lua_setglobal( pLuaState, "PlayerMove" );

	return 1;
}