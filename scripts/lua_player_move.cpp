#include <stdio.h>
#include <data_struct/hashdict.h>
#include <hl_sdk/pm_shared/pm_shared.h>

#include "scripts_binding.h"
#include "lua_player_move.h"

#define PLAYERMOVE_TYPE "playermove"

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

DEFINE_GETTER( PLAYERMOVE_TYPE, playermove_t * );
DEFINE_SETTER( PLAYERMOVE_TYPE, playermove_t * );

DEFINE_SCRIPTFUNC( MetaMethod_tostring )
{
	char s[ 64 ];
	playermove_t *pm = lua_getplayermove( pLuaState, 1 );

	snprintf( s, 64, "(%s : %X)", PLAYERMOVE_TYPE, (unsigned long)pm );

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

LUALIB_API int luaopen_playermove( lua_State *pLuaState )
{
	VLua::RegisterMetaTable( PLAYERMOVE_TYPE );
	VLua::RegisterMetaTableFunctions( PLAYERMOVE_TYPE, Registrations );

	if ( !VLua::ArePropertiesInitialized() )
	{
		VLua::BindProperty( PLAYERMOVE_TYPE, "player_index", VLuaPropertyDesc( playermove_t, player_index ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "server", VLuaPropertyDescFieldtype( playermove_t, server, VLUA_FIELD_TYPE_BOOLEAN ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "multiplayer", VLuaPropertyDescFieldtype( playermove_t, multiplayer, VLUA_FIELD_TYPE_BOOLEAN ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "time", VLuaPropertyDesc( playermove_t, time ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "frametime", VLuaPropertyDesc( playermove_t, frametime ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "forward", VLuaPropertyDesc( playermove_t, forward ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "right", VLuaPropertyDesc( playermove_t, right ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "up", VLuaPropertyDesc( playermove_t, up ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "origin", VLuaPropertyDesc( playermove_t, origin ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "angles", VLuaPropertyDesc( playermove_t, angles ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "oldangles", VLuaPropertyDesc( playermove_t, oldangles ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "velocity", VLuaPropertyDesc( playermove_t, velocity ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "movedir", VLuaPropertyDesc( playermove_t, movedir ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "basevelocity", VLuaPropertyDesc( playermove_t, basevelocity ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "view_ofs", VLuaPropertyDesc( playermove_t, view_ofs ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "flDuckTime", VLuaPropertyDesc( playermove_t, flDuckTime ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "bInDuck", VLuaPropertyDescFieldtype( playermove_t, bInDuck, VLUA_FIELD_TYPE_BOOLEAN ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "flTimeStepSound", VLuaPropertyDesc( playermove_t, flTimeStepSound ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "iStepLeft", VLuaPropertyDesc( playermove_t, iStepLeft ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "flFallVelocity", VLuaPropertyDesc( playermove_t, flFallVelocity ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "punchangle", VLuaPropertyDesc( playermove_t, punchangle ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "flSwimTime", VLuaPropertyDesc( playermove_t, flSwimTime ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "flNextPrimaryAttack", VLuaPropertyDesc( playermove_t, flNextPrimaryAttack ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "effects", VLuaPropertyDesc( playermove_t, effects ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "flags", VLuaPropertyDesc( playermove_t, flags ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "usehull", VLuaPropertyDesc( playermove_t, usehull ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "gravity", VLuaPropertyDesc( playermove_t, gravity ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "friction", VLuaPropertyDesc( playermove_t, friction ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "oldbuttons", VLuaPropertyDesc( playermove_t, oldbuttons ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "waterjumptime", VLuaPropertyDesc( playermove_t, waterjumptime ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "dead", VLuaPropertyDescFieldtype( playermove_t, dead, VLUA_FIELD_TYPE_BOOLEAN ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "deadflag", VLuaPropertyDesc( playermove_t, deadflag ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "spectator", VLuaPropertyDesc( playermove_t, spectator ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "movetype", VLuaPropertyDesc( playermove_t, movetype ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "onground", VLuaPropertyDesc( playermove_t, onground ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "waterlevel", VLuaPropertyDesc( playermove_t, waterlevel ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "watertype", VLuaPropertyDesc( playermove_t, watertype ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "oldwaterlevel", VLuaPropertyDesc( playermove_t, oldwaterlevel ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "sztexturename", VLuaPropertyDescFieldtype( playermove_t, sztexturename, VLUA_FIELD_TYPE_CSTRING ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "chtexturetype", VLuaPropertyDesc( playermove_t, chtexturetype ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "maxspeed", VLuaPropertyDesc( playermove_t, maxspeed ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "clientmaxspeed", VLuaPropertyDesc( playermove_t, clientmaxspeed ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "iuser1", VLuaPropertyDesc( playermove_t, iuser1 ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "iuser2", VLuaPropertyDesc( playermove_t, iuser2 ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "iuser3", VLuaPropertyDesc( playermove_t, iuser3 ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "iuser4", VLuaPropertyDesc( playermove_t, iuser4 ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "fuser1", VLuaPropertyDesc( playermove_t, fuser1 ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "fuser2", VLuaPropertyDesc( playermove_t, fuser2 ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "fuser3", VLuaPropertyDesc( playermove_t, fuser3 ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "fuser4", VLuaPropertyDesc( playermove_t, fuser4 ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "vuser1", VLuaPropertyDesc( playermove_t, vuser1 ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "vuser2", VLuaPropertyDesc( playermove_t, vuser2 ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "vuser3", VLuaPropertyDesc( playermove_t, vuser3 ), true, false );
		VLua::BindProperty( PLAYERMOVE_TYPE, "vuser4", VLuaPropertyDesc( playermove_t, vuser4 ), true, false );
	}

	VLua::RegisterGlobalVariable( "PlayerMove", g_pPlayerMove );

	return 1;
}