#include <stdio.h>
#include <data_struct/hashdict.h>

#include "scripts_binding.h"
#include "lua_global_vars.h"

#include "../modules/server.h"

#define GLOBALVARS_TYPE "globalvars"

//-----------------------------------------------------------------------------
// Access functions
//-----------------------------------------------------------------------------

bool lua_isglobalvars( lua_State *pLuaState, int i )
{
	if ( luaL_checkudata( pLuaState, i, GLOBALVARS_TYPE ) == NULL )
	{
		return false;
	}

	return true;
}

globalvars_t *lua_getglobalvars( lua_State *pLuaState, int i )
{
	if ( luaL_checkudata( pLuaState, i, GLOBALVARS_TYPE ) == NULL )
	{
		luaL_typeerror( pLuaState, i, GLOBALVARS_TYPE );
	}

	return *(globalvars_t **)lua_touserdata( pLuaState, i );
}

void lua_pushglobalvars( lua_State *pLuaState, globalvars_t *pGlobals )
{
	// store pointer
	globalvars_t **globals = (globalvars_t **)lua_newuserdata( pLuaState, sizeof( globalvars_t * ) );

	luaL_getmetatable( pLuaState, GLOBALVARS_TYPE );
	lua_setmetatable( pLuaState, -2 );

	*globals = pGlobals;
}

//-----------------------------------------------------------------------------
// Script functions
//-----------------------------------------------------------------------------

DEFINE_GETTER( GLOBALVARS_TYPE, globalvars_t * );
DEFINE_SETTER( GLOBALVARS_TYPE, globalvars_t * );

DEFINE_SCRIPTFUNC( MetaMethod_tostring )
{
	char s[ 64 ];
	globalvars_t *globals = lua_getglobalvars( pLuaState, 1 );

	snprintf( s, 64, "(%s : %X)", GLOBALVARS_TYPE, (unsigned long)globals );

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

LUALIB_API int luaopen_globalvars( lua_State *pLuaState )
{
	VLua::RegisterMetaTable( GLOBALVARS_TYPE );
	VLua::RegisterMetaTableFunctions( GLOBALVARS_TYPE, Registrations );

	if ( !VLua::ArePropertiesInitialized() )
	{
		VLua::BindProperty( GLOBALVARS_TYPE, "time", VLuaPropertyDesc( globalvars_t, time ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "frametime", VLuaPropertyDesc( globalvars_t, frametime ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "force_retouch", VLuaPropertyDesc( globalvars_t, force_retouch ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "mapname", VLuaPropertyDescFieldtype( globalvars_t, mapname, VLUA_FIELD_TYPE_STRING ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "startspot", VLuaPropertyDescFieldtype( globalvars_t, startspot, VLUA_FIELD_TYPE_STRING ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "deathmatch", VLuaPropertyDesc( globalvars_t, deathmatch ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "coop", VLuaPropertyDesc( globalvars_t, coop ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "teamplay", VLuaPropertyDesc( globalvars_t, teamplay ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "serverflags", VLuaPropertyDesc( globalvars_t, serverflags ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "found_secrets", VLuaPropertyDesc( globalvars_t, found_secrets ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "v_forward", VLuaPropertyDesc( globalvars_t, v_forward ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "v_up", VLuaPropertyDesc( globalvars_t, v_up ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "v_right", VLuaPropertyDesc( globalvars_t, v_right ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "trace_allsolid", VLuaPropertyDesc( globalvars_t, trace_allsolid ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "trace_startsolid", VLuaPropertyDesc( globalvars_t, trace_startsolid ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "trace_fraction", VLuaPropertyDesc( globalvars_t, trace_fraction ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "trace_endpos", VLuaPropertyDesc( globalvars_t, trace_endpos ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "trace_plane_normal", VLuaPropertyDesc( globalvars_t, trace_plane_normal ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "trace_plane_dist", VLuaPropertyDesc( globalvars_t, trace_plane_dist ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "trace_ent", VLuaPropertyDesc( globalvars_t, trace_ent ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "trace_inopen", VLuaPropertyDesc( globalvars_t, trace_inopen ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "trace_inwater", VLuaPropertyDesc( globalvars_t, trace_inwater ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "trace_hitgroup", VLuaPropertyDesc( globalvars_t, trace_hitgroup ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "trace_flags", VLuaPropertyDesc( globalvars_t, trace_flags ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "msg_entity", VLuaPropertyDesc( globalvars_t, msg_entity ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "cdAudioTrack", VLuaPropertyDesc( globalvars_t, cdAudioTrack ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "maxClients", VLuaPropertyDesc( globalvars_t, maxClients ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "maxEntities", VLuaPropertyDesc( globalvars_t, maxEntities ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "pStringBase", VLuaPropertyDescFieldtype( globalvars_t, pStringBase, VLUA_FIELD_TYPE_INTEGER ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "pSaveData", VLuaPropertyDescFieldtype( globalvars_t, pSaveData, VLUA_FIELD_TYPE_INTEGER ), true, false );
		VLua::BindProperty( GLOBALVARS_TYPE, "vecLandmarkOffset", VLuaPropertyDesc( globalvars_t, vecLandmarkOffset ), true, false );
	}

	VLua::RegisterGlobalVariable( "gpGlobals", gpGlobals );

	return 1;
}