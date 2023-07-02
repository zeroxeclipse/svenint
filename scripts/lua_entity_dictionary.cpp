#include <stdio.h>
#include <data_struct/hashdict.h>

#include "scripts_binding.h"
#include "lua_entity_dictionary.h"
#include "lua_entity_vars.h"

#define EDICT_TYPE "edict"

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

DEFINE_GETTER( EDICT_TYPE, edict_t * );
DEFINE_SETTER( EDICT_TYPE, edict_t * );

DEFINE_SCRIPTFUNC( MetaMethod_tostring )
{
	char s[ 64 ];
	edict_t *e = lua_getedict( pLuaState, 1 );

	snprintf( s, 64, "(%s : %X)", EDICT_TYPE, (unsigned long)e );

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

LUALIB_API int luaopen_edict( lua_State *pLuaState )
{
	VLua::RegisterMetaTable( EDICT_TYPE );
	VLua::RegisterMetaTableFunctions( EDICT_TYPE, Registrations );

	if ( !VLua::ArePropertiesInitialized() )
	{
		VLua::BindProperty( EDICT_TYPE, "free", VLuaPropertyDescFieldtype( edict_t, free, VLUA_FIELD_TYPE_BOOLEAN ), true, false );
		VLua::BindProperty( EDICT_TYPE, "headnode", VLuaPropertyDesc( edict_t, headnode ), true, false );
		VLua::BindProperty( EDICT_TYPE, "num_leafs", VLuaPropertyDesc( edict_t, num_leafs ), true, false );
		VLua::BindProperty( EDICT_TYPE, "freetime", VLuaPropertyDesc( edict_t, freetime ), true, false );
		VLua::BindProperty( EDICT_TYPE, "pvPrivateData", VLuaPropertyDescFieldtype( edict_t, pvPrivateData, VLUA_FIELD_TYPE_INTEGER ), true, false );
		VLua::BindProperty( EDICT_TYPE, "vars", { VLUA_FIELD_TYPE_ENTVARS, VLuaOffsetOf( edict_t, v ), (int)sizeof( edict_t * ), true }, true, false );
	}

	return 1;
}