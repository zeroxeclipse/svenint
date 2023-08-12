#include <stdio.h>
#include <data_struct/hashdict.h>

#include "scripts_binding.h"
#include "lua_vector.h"
#include "lua_entity_vars.h"

#include "../modules/server.h"

#define ENTVARS_TYPE "entvars"

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

DEFINE_GETTER( ENTVARS_TYPE, entvars_t * );
DEFINE_SETTER( ENTVARS_TYPE, entvars_t * );

DEFINE_SCRIPTFUNC( MetaMethod_tostring )
{
	char s[ 64 ];
	entvars_t *entvars = lua_getentvars( pLuaState, 1 );

	snprintf( s, 64, "(%s : %X)", ENTVARS_TYPE, (unsigned long)entvars );

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

LUALIB_API int luaopen_entvars( lua_State *pLuaState )
{
	VLua::RegisterMetaTable( ENTVARS_TYPE );
	VLua::RegisterMetaTableFunctions( ENTVARS_TYPE, Registrations );

	if ( !VLua::ArePropertiesInitialized() )
	{
		VLua::BindProperty( ENTVARS_TYPE, "classname", VLuaPropertyDescFieldtype( entvars_t, classname, VLUA_FIELD_TYPE_STRING ), true, false );
		VLua::BindProperty( ENTVARS_TYPE, "globalname", VLuaPropertyDescFieldtype( entvars_t, globalname, VLUA_FIELD_TYPE_STRING ), true, false );
		VLua::BindProperty( ENTVARS_TYPE, "origin", VLuaPropertyDesc( entvars_t, origin ) );
		VLua::BindProperty( ENTVARS_TYPE, "oldorigin", VLuaPropertyDesc( entvars_t, oldorigin ) );
		VLua::BindProperty( ENTVARS_TYPE, "velocity", VLuaPropertyDesc( entvars_t, velocity ) );
		VLua::BindProperty( ENTVARS_TYPE, "basevelocity", VLuaPropertyDesc( entvars_t, basevelocity ) );
		VLua::BindProperty( ENTVARS_TYPE, "clbasevelocity", VLuaPropertyDesc( entvars_t, clbasevelocity ) );
		VLua::BindProperty( ENTVARS_TYPE, "movedir", VLuaPropertyDesc( entvars_t, movedir ) );
		VLua::BindProperty( ENTVARS_TYPE, "angles", VLuaPropertyDesc( entvars_t, angles ) );
		VLua::BindProperty( ENTVARS_TYPE, "avelocity", VLuaPropertyDesc( entvars_t, avelocity ) );
		VLua::BindProperty( ENTVARS_TYPE, "punchangle", VLuaPropertyDesc( entvars_t, punchangle ) );
		VLua::BindProperty( ENTVARS_TYPE, "v_angle", VLuaPropertyDesc( entvars_t, v_angle ) );
		VLua::BindProperty( ENTVARS_TYPE, "endpos", VLuaPropertyDesc( entvars_t, endpos ) );
		VLua::BindProperty( ENTVARS_TYPE, "startpos", VLuaPropertyDesc( entvars_t, startpos ) );
		VLua::BindProperty( ENTVARS_TYPE, "impacttime", VLuaPropertyDesc( entvars_t, impacttime ) );
		VLua::BindProperty( ENTVARS_TYPE, "starttime", VLuaPropertyDesc( entvars_t, starttime ) );
		VLua::BindProperty( ENTVARS_TYPE, "fixangle", VLuaPropertyDesc( entvars_t, fixangle ) );
		VLua::BindProperty( ENTVARS_TYPE, "idealpitch", VLuaPropertyDesc( entvars_t, idealpitch ) );
		VLua::BindProperty( ENTVARS_TYPE, "pitch_speed", VLuaPropertyDesc( entvars_t, pitch_speed ) );
		VLua::BindProperty( ENTVARS_TYPE, "ideal_yaw", VLuaPropertyDesc( entvars_t, ideal_yaw ) );
		VLua::BindProperty( ENTVARS_TYPE, "yaw_speed", VLuaPropertyDesc( entvars_t, yaw_speed ) );
		VLua::BindProperty( ENTVARS_TYPE, "modelindex", VLuaPropertyDesc( entvars_t, modelindex ) );
		VLua::BindProperty( ENTVARS_TYPE, "model", VLuaPropertyDescFieldtype( entvars_t, model, VLUA_FIELD_TYPE_STRING ), true, false );
		VLua::BindProperty( ENTVARS_TYPE, "viewmodel", VLuaPropertyDesc( entvars_t, viewmodel ) );
		VLua::BindProperty( ENTVARS_TYPE, "weaponmodel", VLuaPropertyDesc( entvars_t, weaponmodel ) );
		VLua::BindProperty( ENTVARS_TYPE, "absmin", VLuaPropertyDesc( entvars_t, absmin ) );
		VLua::BindProperty( ENTVARS_TYPE, "absmax", VLuaPropertyDesc( entvars_t, absmax ) );
		VLua::BindProperty( ENTVARS_TYPE, "mins", VLuaPropertyDesc( entvars_t, mins ) );
		VLua::BindProperty( ENTVARS_TYPE, "maxs", VLuaPropertyDesc( entvars_t, maxs ) );
		VLua::BindProperty( ENTVARS_TYPE, "size", VLuaPropertyDesc( entvars_t, size ) );
		VLua::BindProperty( ENTVARS_TYPE, "ltime", VLuaPropertyDesc( entvars_t, ltime ) );
		VLua::BindProperty( ENTVARS_TYPE, "nextthink", VLuaPropertyDesc( entvars_t, nextthink ) );
		VLua::BindProperty( ENTVARS_TYPE, "movetype", VLuaPropertyDesc( entvars_t, movetype ) );
		VLua::BindProperty( ENTVARS_TYPE, "solid", VLuaPropertyDesc( entvars_t, solid ) );
		VLua::BindProperty( ENTVARS_TYPE, "skin", VLuaPropertyDesc( entvars_t, skin ) );
		VLua::BindProperty( ENTVARS_TYPE, "body", VLuaPropertyDesc( entvars_t, body ) );
		VLua::BindProperty( ENTVARS_TYPE, "effects", VLuaPropertyDesc( entvars_t, effects ) );
		VLua::BindProperty( ENTVARS_TYPE, "gravity", VLuaPropertyDesc( entvars_t, gravity ) );
		VLua::BindProperty( ENTVARS_TYPE, "friction", VLuaPropertyDesc( entvars_t, friction ) );
		VLua::BindProperty( ENTVARS_TYPE, "light_level", VLuaPropertyDesc( entvars_t, light_level ) );
		VLua::BindProperty( ENTVARS_TYPE, "sequence", VLuaPropertyDesc( entvars_t, sequence ) );
		VLua::BindProperty( ENTVARS_TYPE, "gaitsequence", VLuaPropertyDesc( entvars_t, gaitsequence ) );
		VLua::BindProperty( ENTVARS_TYPE, "frame", VLuaPropertyDesc( entvars_t, frame ) );
		VLua::BindProperty( ENTVARS_TYPE, "animtime", VLuaPropertyDesc( entvars_t, animtime ) );
		VLua::BindProperty( ENTVARS_TYPE, "framerate", VLuaPropertyDesc( entvars_t, framerate ) );
		//VLua::BindProperty( ENTVARS_TYPE, "controller", VLuaPropertyDesc( entvars_t, controller ) );
		//VLua::BindProperty( ENTVARS_TYPE, "blending", VLuaPropertyDesc( entvars_t, blending ) );
		VLua::BindProperty( ENTVARS_TYPE, "scale", VLuaPropertyDesc( entvars_t, scale ) );
		VLua::BindProperty( ENTVARS_TYPE, "rendermode", VLuaPropertyDesc( entvars_t, rendermode ) );
		VLua::BindProperty( ENTVARS_TYPE, "renderamt", VLuaPropertyDesc( entvars_t, renderamt ) );
		VLua::BindProperty( ENTVARS_TYPE, "rendercolor", VLuaPropertyDesc( entvars_t, rendercolor ) );
		VLua::BindProperty( ENTVARS_TYPE, "renderfx", VLuaPropertyDesc( entvars_t, renderfx ) );
		VLua::BindProperty( ENTVARS_TYPE, "health", VLuaPropertyDesc( entvars_t, health ) );
		VLua::BindProperty( ENTVARS_TYPE, "frags", VLuaPropertyDesc( entvars_t, frags ) );
		VLua::BindProperty( ENTVARS_TYPE, "weapons", VLuaPropertyDesc( entvars_t, weapons ) );
		VLua::BindProperty( ENTVARS_TYPE, "takedamage", VLuaPropertyDesc( entvars_t, takedamage ) );
		VLua::BindProperty( ENTVARS_TYPE, "deadflag", VLuaPropertyDesc( entvars_t, deadflag ) );
		VLua::BindProperty( ENTVARS_TYPE, "view_ofs", VLuaPropertyDesc( entvars_t, view_ofs ) );
		VLua::BindProperty( ENTVARS_TYPE, "button", VLuaPropertyDesc( entvars_t, button ) );
		VLua::BindProperty( ENTVARS_TYPE, "impulse", VLuaPropertyDesc( entvars_t, impulse ) );
		VLua::BindProperty( ENTVARS_TYPE, "chain", VLuaPropertyDesc( entvars_t, chain ) );
		VLua::BindProperty( ENTVARS_TYPE, "dmg_inflictor", VLuaPropertyDesc( entvars_t, dmg_inflictor ) );
		VLua::BindProperty( ENTVARS_TYPE, "enemy", VLuaPropertyDesc( entvars_t, enemy ) );
		VLua::BindProperty( ENTVARS_TYPE, "aiment", VLuaPropertyDesc( entvars_t, aiment ) );
		VLua::BindProperty( ENTVARS_TYPE, "owner", VLuaPropertyDesc( entvars_t, owner ) );
		VLua::BindProperty( ENTVARS_TYPE, "groundentity", VLuaPropertyDesc( entvars_t, groundentity ) );
		VLua::BindProperty( ENTVARS_TYPE, "spawnflags", VLuaPropertyDesc( entvars_t, spawnflags ) );
		VLua::BindProperty( ENTVARS_TYPE, "flags", VLuaPropertyDesc( entvars_t, flags ) );
		VLua::BindProperty( ENTVARS_TYPE, "colormap", VLuaPropertyDesc( entvars_t, colormap ) );
		VLua::BindProperty( ENTVARS_TYPE, "team", VLuaPropertyDesc( entvars_t, team ) );
		VLua::BindProperty( ENTVARS_TYPE, "max_health", VLuaPropertyDesc( entvars_t, max_health ) );
		VLua::BindProperty( ENTVARS_TYPE, "teleport_time", VLuaPropertyDesc( entvars_t, teleport_time ) );
		VLua::BindProperty( ENTVARS_TYPE, "armortype", VLuaPropertyDesc( entvars_t, armortype ) );
		VLua::BindProperty( ENTVARS_TYPE, "armorvalue", VLuaPropertyDesc( entvars_t, armorvalue ) );
		VLua::BindProperty( ENTVARS_TYPE, "waterlevel", VLuaPropertyDesc( entvars_t, waterlevel ) );
		VLua::BindProperty( ENTVARS_TYPE, "watertype", VLuaPropertyDesc( entvars_t, watertype ) );
		VLua::BindProperty( ENTVARS_TYPE, "target", VLuaPropertyDescFieldtype( entvars_t, target, VLUA_FIELD_TYPE_STRING ), true, false );
		VLua::BindProperty( ENTVARS_TYPE, "targetname", VLuaPropertyDescFieldtype( entvars_t, targetname, VLUA_FIELD_TYPE_STRING ), true, true ); // true, false
		VLua::BindProperty( ENTVARS_TYPE, "netname", VLuaPropertyDescFieldtype( entvars_t, netname, VLUA_FIELD_TYPE_STRING ), true, false );
		VLua::BindProperty( ENTVARS_TYPE, "message", VLuaPropertyDescFieldtype( entvars_t, message, VLUA_FIELD_TYPE_STRING ), true, false );
		VLua::BindProperty( ENTVARS_TYPE, "dmg_take", VLuaPropertyDesc( entvars_t, dmg_take ) );
		VLua::BindProperty( ENTVARS_TYPE, "dmg_save", VLuaPropertyDesc( entvars_t, dmg_save ) );
		VLua::BindProperty( ENTVARS_TYPE, "dmg", VLuaPropertyDesc( entvars_t, dmg ) );
		VLua::BindProperty( ENTVARS_TYPE, "dmgtime", VLuaPropertyDesc( entvars_t, dmgtime ) );
		VLua::BindProperty( ENTVARS_TYPE, "noise", VLuaPropertyDescFieldtype( entvars_t, noise, VLUA_FIELD_TYPE_STRING ), true, false );
		VLua::BindProperty( ENTVARS_TYPE, "noise1", VLuaPropertyDescFieldtype( entvars_t, noise1, VLUA_FIELD_TYPE_STRING ), true, false );
		VLua::BindProperty( ENTVARS_TYPE, "noise2", VLuaPropertyDescFieldtype( entvars_t, noise2, VLUA_FIELD_TYPE_STRING ), true, false );
		VLua::BindProperty( ENTVARS_TYPE, "noise3", VLuaPropertyDescFieldtype( entvars_t, noise3, VLUA_FIELD_TYPE_STRING ), true, false );
		VLua::BindProperty( ENTVARS_TYPE, "speed", VLuaPropertyDesc( entvars_t, speed ) );
		VLua::BindProperty( ENTVARS_TYPE, "air_finished", VLuaPropertyDesc( entvars_t, air_finished ) );
		VLua::BindProperty( ENTVARS_TYPE, "pain_finished", VLuaPropertyDesc( entvars_t, pain_finished ) );
		VLua::BindProperty( ENTVARS_TYPE, "radsuit_finished", VLuaPropertyDesc( entvars_t, radsuit_finished ) );
		VLua::BindProperty( ENTVARS_TYPE, "pContainingEntity", VLuaPropertyDesc( entvars_t, pContainingEntity ) );
		VLua::BindProperty( ENTVARS_TYPE, "playerclass", VLuaPropertyDesc( entvars_t, playerclass ) );
		VLua::BindProperty( ENTVARS_TYPE, "maxspeed", VLuaPropertyDesc( entvars_t, maxspeed ) );
		VLua::BindProperty( ENTVARS_TYPE, "fov", VLuaPropertyDesc( entvars_t, fov ) );
		VLua::BindProperty( ENTVARS_TYPE, "weaponanim", VLuaPropertyDesc( entvars_t, weaponanim ) );
		VLua::BindProperty( ENTVARS_TYPE, "pushmsec", VLuaPropertyDesc( entvars_t, pushmsec ) );
		VLua::BindProperty( ENTVARS_TYPE, "bInDuck", VLuaPropertyDesc( entvars_t, bInDuck ) );
		VLua::BindProperty( ENTVARS_TYPE, "flTimeStepSound", VLuaPropertyDesc( entvars_t, flTimeStepSound ) );
		VLua::BindProperty( ENTVARS_TYPE, "flSwimTime", VLuaPropertyDesc( entvars_t, flSwimTime ) );
		VLua::BindProperty( ENTVARS_TYPE, "flDuckTime", VLuaPropertyDesc( entvars_t, flDuckTime ) );
		VLua::BindProperty( ENTVARS_TYPE, "iStepLeft", VLuaPropertyDesc( entvars_t, iStepLeft ) );
		VLua::BindProperty( ENTVARS_TYPE, "flFallVelocity", VLuaPropertyDesc( entvars_t, flFallVelocity ) );
		VLua::BindProperty( ENTVARS_TYPE, "gamestate", VLuaPropertyDesc( entvars_t, gamestate ) );
		VLua::BindProperty( ENTVARS_TYPE, "oldbuttons", VLuaPropertyDesc( entvars_t, oldbuttons ) );
		VLua::BindProperty( ENTVARS_TYPE, "groupinfo", VLuaPropertyDesc( entvars_t, groupinfo ) );
		VLua::BindProperty( ENTVARS_TYPE, "iuser1", VLuaPropertyDesc( entvars_t, iuser1 ) );
		VLua::BindProperty( ENTVARS_TYPE, "iuser2", VLuaPropertyDesc( entvars_t, iuser2 ) );
		VLua::BindProperty( ENTVARS_TYPE, "iuser3", VLuaPropertyDesc( entvars_t, iuser3 ) );
		VLua::BindProperty( ENTVARS_TYPE, "iuser4", VLuaPropertyDesc( entvars_t, iuser4 ) );
		VLua::BindProperty( ENTVARS_TYPE, "fuser1", VLuaPropertyDesc( entvars_t, fuser1 ) );
		VLua::BindProperty( ENTVARS_TYPE, "fuser2", VLuaPropertyDesc( entvars_t, fuser2 ) );
		VLua::BindProperty( ENTVARS_TYPE, "fuser3", VLuaPropertyDesc( entvars_t, fuser3 ) );
		VLua::BindProperty( ENTVARS_TYPE, "fuser4", VLuaPropertyDesc( entvars_t, fuser4 ) );
		VLua::BindProperty( ENTVARS_TYPE, "vuser1", VLuaPropertyDesc( entvars_t, vuser1 ) );
		VLua::BindProperty( ENTVARS_TYPE, "vuser2", VLuaPropertyDesc( entvars_t, vuser2 ) );
		VLua::BindProperty( ENTVARS_TYPE, "vuser3", VLuaPropertyDesc( entvars_t, vuser3 ) );
		VLua::BindProperty( ENTVARS_TYPE, "vuser4", VLuaPropertyDesc( entvars_t, vuser4 ) );
		VLua::BindProperty( ENTVARS_TYPE, "euser1", VLuaPropertyDesc( entvars_t, euser1 ) );
		VLua::BindProperty( ENTVARS_TYPE, "euser2", VLuaPropertyDesc( entvars_t, euser2 ) );
		VLua::BindProperty( ENTVARS_TYPE, "euser3", VLuaPropertyDesc( entvars_t, euser3 ) );
		VLua::BindProperty( ENTVARS_TYPE, "euser4", VLuaPropertyDesc( entvars_t, euser4 ) );
	}

	return 1;
}