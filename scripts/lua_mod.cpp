#include "lua_mod.h"
#include "lua_vector.h"
#include "lua_usercmd.h"
#include "lua_entity_dictionary.h"
#include "scripts.h"
#include "scripts_binding.h"

#include "../modules/server.h"
#include "../modules/server_client_bridge.h"

#include "../features/aim.h"
#include "../features/misc.h"
#include "../features/speedrun_tools.h"

#include <string>

#include <dbg.h>
#include <ISvenModAPI.h>
#include <hl_sdk/engine/APIProxy.h>

//-----------------------------------------------------------------------------
// Get mapname helper
//-----------------------------------------------------------------------------

static const char *GetMapName()
{
	static char mapname_buffer[ MAX_PATH ];

	char *pszMapName = mapname_buffer;
	char *pszExt = NULL;

	strncpy( mapname_buffer, g_pEngineFuncs->GetLevelName(), MAX_PATH );

	// maps/<mapname>.bsp to <mapname>
	while ( *pszMapName )
	{
		if ( *pszMapName == '/' )
		{
			pszMapName++;
			break;
		}

		pszMapName++;
	}

	pszExt = pszMapName;

	while ( *pszExt )
	{
		if ( *pszExt == '.' )
		{
			*pszExt = 0;
			break;
		}

		pszExt++;
	}

	char *tmp = pszMapName;
	while ( *tmp )
	{
		*tmp = tolower( *tmp );
		tmp++;
	}

	return pszMapName;
}

//-----------------------------------------------------------------------------
// C to Lua
//-----------------------------------------------------------------------------

DEFINE_SCRIPTFUNC( IncludeScript )
{
	const char *pszFilename = lua_tostring( pLuaState, 1 );

	auto ends_with = []( std::string const &value, std::string const &ending ) -> bool
	{
		if ( ending.size() > value.size() )
			return false;

		return std::equal( ending.rbegin(), ending.rend(), value.rbegin() );
	};

	std::string sFilePath = SvenModAPI()->GetBaseDirectory();

	sFilePath += "\\sven_internal\\scripts\\";
	sFilePath += pszFilename;

	if ( !ends_with( pszFilename, ".lua" ) )
	{
		sFilePath += ".lua";
	}

	g_ScriptVM.RunScriptFile( sFilePath.c_str() );

	return VLUA_RET_ARGS( 0 );
}

DEFINE_SCRIPTFUNC( GetMapName )
{
	lua_pushstring( pLuaState, GetMapName() );

	return VLUA_RET_ARGS( 1 );
}

DEFINE_SCRIPTFUNC( ClientCmd )
{
	const char *pszConCommand = lua_tostring( pLuaState, 1 );
	g_pEngineFuncs->ClientCmd( pszConCommand );

	return VLUA_RET_ARGS( 0 );
}

DEFINE_SCRIPTFUNC( PrintChatText )
{
	static char buffer[ 1024 ];

	const char *pszText = lua_tostring( pLuaState, 1 );

	strncpy( buffer, pszText, 1023 );
	buffer[ 1023 ] = 0;

	Utils()->PrintChatText( pszText );

	return VLUA_RET_ARGS( 0 );
}

DEFINE_SCRIPTFUNC( GetPEntityFromEntityIndex )
{
	if ( !Host_IsServerActive() )
	{
		lua_pushnil( pLuaState );
		return VLUA_RET_ARGS( 1 );
	}

	int iEntIndex = (int)lua_tointeger( pLuaState, 1 );

	edict_t *pEdict = g_pServerEngineFuncs->pfnPEntityOfEntIndex( iEntIndex );

	if ( pEdict != NULL )
	{
		lua_pushedict( pLuaState, pEdict );
	}
	else
	{
		lua_pushnil( pLuaState );
	}

	return VLUA_RET_ARGS( 1 );
}

DEFINE_SCRIPTFUNC( GetEntityIndexFromEdict )
{
	if ( !Host_IsServerActive() )
	{
		lua_pushinteger( pLuaState, -1 );
		return VLUA_RET_ARGS( 1 );
	}

	edict_t *pEdict = lua_getedict( pLuaState, 1 );

	lua_pushinteger( pLuaState, (lua_Integer)g_pServerEngineFuncs->pfnIndexOfEdict( pEdict ) );

	return VLUA_RET_ARGS( 1 );
}

DEFINE_SCRIPTFUNC( SendCommandToClient )
{
	if ( !Host_IsServerActive() )
		return VLUA_RET_ARGS( 0 );

	edict_t *ed = lua_getedict( pLuaState, 1 );
	const char *command = lua_tostring( pLuaState, 2 );

	g_pServerEngineFuncs->pfnMessageBegin( MSG_ONE, SVC_SVENINT, NULL, ed );
		g_pServerEngineFuncs->pfnWriteByte( SVENINT_COMM_EXECUTE );
		g_pServerEngineFuncs->pfnWriteString( command );
	g_pServerEngineFuncs->pfnMessageEnd();

	return VLUA_RET_ARGS( 0 );
}

DEFINE_SCRIPTFUNC( SendSignalToClient )
{
	if ( !Host_IsServerActive() )
		return VLUA_RET_ARGS( 0 );

	edict_t *ed = lua_getedict( pLuaState, 1 );
	int value = (int)lua_tointeger( pLuaState, 2 );

	g_pServerEngineFuncs->pfnMessageBegin( MSG_ONE, SVC_SVENINT, NULL, ed );
		g_pServerEngineFuncs->pfnWriteByte( SVENINT_COMM_SCRIPTS );
		g_pServerEngineFuncs->pfnWriteByte( 0 ); // signal from server
		g_pServerEngineFuncs->pfnWriteLong( value );
	g_pServerEngineFuncs->pfnMessageEnd();

	return VLUA_RET_ARGS( 0 );
}

DEFINE_SCRIPTFUNC( IsListenServer )
{
	lua_pushboolean( pLuaState, (int)Host_IsServerActive() );

	return VLUA_RET_ARGS( 1 );
}

DEFINE_SCRIPTFUNC( IsSurvivalModeEnabled )
{
	lua_pushboolean( pLuaState, (int)IsSurvivalModeEnabled() );

	return VLUA_RET_ARGS( 1 );
}

DEFINE_SCRIPTFUNC( EnableSurvivalMode )
{
	lua_pushboolean( pLuaState, (int)EnableSurvivalMode() );

	return VLUA_RET_ARGS( 1 );
}

DEFINE_SCRIPTFUNC( DisableSurvivalMode )
{
	lua_pushboolean( pLuaState, (int)DisableSurvivalMode() );

	return VLUA_RET_ARGS( 1 );
}

DEFINE_SCRIPTFUNC( SetTimescale )
{
	float timescale = (float)lua_tonumber( pLuaState, 1 );

	g_SpeedrunTools.SetTimescale( timescale );

	return VLUA_RET_ARGS( 0 );
}

DEFINE_SCRIPTFUNC( StartTimer )
{
	g_SpeedrunTools.StartTimer();

	return VLUA_RET_ARGS( 0 );
}

DEFINE_SCRIPTFUNC( StopTimer )
{
	g_SpeedrunTools.StopTimer();

	return VLUA_RET_ARGS( 0 );
}

DEFINE_SCRIPTFUNC( SegmentCurrentTime )
{
	lua_pushnumber( pLuaState, (lua_Number)g_SpeedrunTools.SegmentCurrentTime() );

	return VLUA_RET_ARGS( 1 );
}

DEFINE_SCRIPTFUNC( LookAt )
{
	Vector vecAngles, vecDir;

	Vector *vecPoint = lua_getvector( pLuaState, 1 );

	vecDir = *vecPoint - ( Client()->GetOrigin() + Client()->GetViewOffset() );

	vecAngles.x = -atan2f( vecDir.z, vecDir.Length2D() ) * (float)( 180.0 / M_PI );
	vecAngles.y = atan2f( vecDir.y, vecDir.x ) * (float)( 180.0 / M_PI );
	vecAngles.z = 0.f;

	g_pEngineFuncs->SetViewAngles( vecAngles );

	return VLUA_RET_ARGS( 0 );
}

DEFINE_SCRIPTFUNC( SetViewAngles )
{
	Vector *va = lua_getvector( pLuaState, 1 );

	g_pEngineFuncs->SetViewAngles( *va );

	return VLUA_RET_ARGS( 0 );
}

DEFINE_SCRIPTFUNC( GetViewAngles )
{
	Vector va;

	g_pEngineFuncs->GetViewAngles( va );

	lua_newvector( pLuaState, &va );

	return VLUA_RET_ARGS( 1 );
}

DEFINE_SCRIPTFUNC( Aimbot )
{
	usercmd_t *cmd = lua_getusercmd( pLuaState, 1 );
	bool bAimbot = lua_toboolean( pLuaState, 2 );
	bool bSilentAimbot = lua_toboolean( pLuaState, 3 );
	bool bRagebot = lua_toboolean( pLuaState, 4 );
	bool bChangeAnglesBack = lua_toboolean( pLuaState, 5 );

	bool bAnglesChanged = false;

	g_Aim.Aimbot( cmd, bAimbot, bSilentAimbot, bRagebot, bChangeAnglesBack, bAnglesChanged );

	if ( !bAnglesChanged && g_Misc.m_bSpinnerDelayed )
	{
		g_Misc.Spinner( cmd );
	}

	return VLUA_RET_ARGS( 0 );
}

//-----------------------------------------------------------------------------
// Init lib
//-----------------------------------------------------------------------------

void lua_setcurrentmap( lua_State *pLuaState )
{
	// Current map name
	VLua::RegisterGlobalVariable( "MapName", GetMapName() );
}

LUALIB_API int luaopen_mod( lua_State *pLuaState )
{
	lua_setcurrentmap( pLuaState );

	// Functions
	VLua::RegisterFunction( "IncludeScript", SCRIPTFUNC( IncludeScript ) );
	VLua::RegisterFunction( "GetMapName", SCRIPTFUNC( GetMapName ) );
	VLua::RegisterFunction( "ClientCmd", SCRIPTFUNC( ClientCmd ) );
	VLua::RegisterFunction( "PrintChatText", SCRIPTFUNC( PrintChatText ) );
	VLua::RegisterFunction( "GetPEntityFromEntityIndex", SCRIPTFUNC( GetPEntityFromEntityIndex ) );
	VLua::RegisterFunction( "GetEntityIndexFromEdict", SCRIPTFUNC( GetEntityIndexFromEdict ) );
	VLua::RegisterFunction( "SendCommandToClient", SCRIPTFUNC( SendCommandToClient ) );
	VLua::RegisterFunction( "SendSignalToClient", SCRIPTFUNC( SendSignalToClient ) );
	VLua::RegisterFunction( "IsListenServer", SCRIPTFUNC( IsListenServer ) );
	VLua::RegisterFunction( "IsSurvivalModeEnabled", SCRIPTFUNC( IsSurvivalModeEnabled ) );
	VLua::RegisterFunction( "EnableSurvivalMode", SCRIPTFUNC( EnableSurvivalMode ) );
	VLua::RegisterFunction( "DisableSurvivalMode", SCRIPTFUNC( DisableSurvivalMode ) );
	VLua::RegisterFunction( "SetTimescale", SCRIPTFUNC( SetTimescale ) );
	VLua::RegisterFunction( "StartTimer", SCRIPTFUNC( StartTimer ) );
	VLua::RegisterFunction( "StopTimer", SCRIPTFUNC( StopTimer ) );
	VLua::RegisterFunction( "SegmentCurrentTime", SCRIPTFUNC( SegmentCurrentTime ) );
	VLua::RegisterFunction( "LookAt", SCRIPTFUNC( LookAt ) );
	VLua::RegisterFunction( "SetViewAngles", SCRIPTFUNC( SetViewAngles ) );
	VLua::RegisterFunction( "GetViewAngles", SCRIPTFUNC( GetViewAngles ) );
	VLua::RegisterFunction( "Aimbot", SCRIPTFUNC( Aimbot ) );

	// Max clients
	VLua::RegisterGlobalVariable( "MaxClients", gpGlobals->maxClients );

	// Client state
	VLua::RegisterGlobalVariable<int>( "CLS_NONE", client_state_t::CLS_NONE );
	VLua::RegisterGlobalVariable<int>( "CLS_DISCONNECTED", client_state_t::CLS_DISCONNECTED );
	VLua::RegisterGlobalVariable<int>( "CLS_CHALLENGE", client_state_t::CLS_CHALLENGE );
	VLua::RegisterGlobalVariable<int>( "CLS_CONNECTED", client_state_t::CLS_CONNECTED );
	VLua::RegisterGlobalVariable<int>( "CLS_LOADING", client_state_t::CLS_LOADING );
	VLua::RegisterGlobalVariable<int>( "CLS_ACTIVE", client_state_t::CLS_ACTIVE );

	// Input buttons
	VLua::RegisterGlobalVariable( "IN_ATTACK", IN_ATTACK );
	VLua::RegisterGlobalVariable( "IN_JUMP", IN_JUMP );
	VLua::RegisterGlobalVariable( "IN_DUCK", IN_DUCK );
	VLua::RegisterGlobalVariable( "IN_FORWARD", IN_FORWARD );
	VLua::RegisterGlobalVariable( "IN_BACK", IN_BACK );
	VLua::RegisterGlobalVariable( "IN_USE", IN_USE );
	VLua::RegisterGlobalVariable( "IN_CANCEL", IN_CANCEL );
	VLua::RegisterGlobalVariable( "IN_LEFT", IN_LEFT );
	VLua::RegisterGlobalVariable( "IN_RIGHT", IN_RIGHT );
	VLua::RegisterGlobalVariable( "IN_MOVELEFT", IN_MOVELEFT );
	VLua::RegisterGlobalVariable( "IN_MOVERIGHT", IN_MOVERIGHT );
	VLua::RegisterGlobalVariable( "IN_ATTACK2", IN_ATTACK2 );
	VLua::RegisterGlobalVariable( "IN_RUN", IN_RUN );
	VLua::RegisterGlobalVariable( "IN_RELOAD", IN_RELOAD );
	VLua::RegisterGlobalVariable( "IN_ALT1", IN_ALT1 );
	VLua::RegisterGlobalVariable( "IN_SCORE", IN_SCORE );
	
	// Flags
	VLua::RegisterGlobalVariable( "FL_FLY", FL_FLY );
	VLua::RegisterGlobalVariable( "FL_SWIM", FL_SWIM );
	VLua::RegisterGlobalVariable( "FL_CONVEYOR", FL_CONVEYOR );
	VLua::RegisterGlobalVariable( "FL_CLIENT", FL_CLIENT );
	VLua::RegisterGlobalVariable( "FL_INWATER", FL_INWATER );
	VLua::RegisterGlobalVariable( "FL_MONSTER", FL_MONSTER );
	VLua::RegisterGlobalVariable( "FL_GODMODE", FL_GODMODE );
	VLua::RegisterGlobalVariable( "FL_NOTARGET", FL_NOTARGET );
	VLua::RegisterGlobalVariable( "FL_SKIPLOCALHOST", FL_SKIPLOCALHOST );
	VLua::RegisterGlobalVariable( "FL_ONGROUND", FL_ONGROUND );
	VLua::RegisterGlobalVariable( "FL_PARTIALGROUND", FL_PARTIALGROUND );
	VLua::RegisterGlobalVariable( "FL_PARTIALFL_WATERJUMPGROUND", FL_WATERJUMP );
	VLua::RegisterGlobalVariable( "FL_FROZEN", FL_FROZEN );
	VLua::RegisterGlobalVariable( "FL_FAKECLIENT", FL_FAKECLIENT );
	VLua::RegisterGlobalVariable( "FL_DUCKING", FL_DUCKING );
	VLua::RegisterGlobalVariable( "FL_FLOAT", FL_FLOAT );
	VLua::RegisterGlobalVariable( "FL_GRAPHED", FL_GRAPHED );
	VLua::RegisterGlobalVariable( "FL_IMMUNE_WATER", FL_IMMUNE_WATER );
	VLua::RegisterGlobalVariable( "FL_IMMUNE_SLIME", FL_IMMUNE_SLIME );
	VLua::RegisterGlobalVariable( "FL_IMMUNE_LAVA", FL_IMMUNE_LAVA );
	VLua::RegisterGlobalVariable( "FL_PROXY", FL_PROXY );
	VLua::RegisterGlobalVariable( "FL_ALWAYSTHINK", FL_ALWAYSTHINK );
	VLua::RegisterGlobalVariable( "FL_BASEVELOCITY", FL_BASEVELOCITY );
	VLua::RegisterGlobalVariable( "FL_MONSTERCLIP", FL_MONSTERCLIP );
	VLua::RegisterGlobalVariable( "FL_ONTRAIN", FL_ONTRAIN );
	VLua::RegisterGlobalVariable( "FL_WORLDBRUSH", FL_WORLDBRUSH );
	VLua::RegisterGlobalVariable( "FL_SPECTATOR", FL_SPECTATOR );
	VLua::RegisterGlobalVariable( "FL_CUSTOMENTITY", FL_CUSTOMENTITY );
	VLua::RegisterGlobalVariable( "FL_KILLME", FL_KILLME );
	VLua::RegisterGlobalVariable( "FL_DORMANT", FL_DORMANT );

	// Water Level
	VLua::RegisterGlobalVariable( "WL_NOT_IN_WATER", WL_NOT_IN_WATER );
	VLua::RegisterGlobalVariable( "WL_FEET", WL_FEET );
	VLua::RegisterGlobalVariable( "WL_WAIST", WL_WAIST );
	VLua::RegisterGlobalVariable( "WL_EYES", WL_EYES );

	// Move Type
	VLua::RegisterGlobalVariable( "MOVETYPE_NONE", MOVETYPE_NONE );
	VLua::RegisterGlobalVariable( "MOVETYPE_ANGLENOCLIP", 1 );
	VLua::RegisterGlobalVariable( "MOVETYPE_ANGLECLIP", 2 );
	VLua::RegisterGlobalVariable( "MOVETYPE_WALK", MOVETYPE_WALK );
	VLua::RegisterGlobalVariable( "MOVETYPE_STEP", MOVETYPE_STEP );
	VLua::RegisterGlobalVariable( "MOVETYPE_FLY", MOVETYPE_FLY );
	VLua::RegisterGlobalVariable( "MOVETYPE_TOSS", MOVETYPE_TOSS );
	VLua::RegisterGlobalVariable( "MOVETYPE_PUSH", MOVETYPE_PUSH );
	VLua::RegisterGlobalVariable( "MOVETYPE_NOCLIP", MOVETYPE_NOCLIP );
	VLua::RegisterGlobalVariable( "MOVETYPE_FLYMISSILE", MOVETYPE_FLYMISSILE );
	VLua::RegisterGlobalVariable( "MOVETYPE_BOUNCE", MOVETYPE_BOUNCE );
	VLua::RegisterGlobalVariable( "MOVETYPE_BOUNCEMISSILE", MOVETYPE_BOUNCEMISSILE );
	VLua::RegisterGlobalVariable( "MOVETYPE_FOLLOW", MOVETYPE_FOLLOW );
	VLua::RegisterGlobalVariable( "MOVETYPE_PUSHSTEP", MOVETYPE_PUSHSTEP );
	
	// Observe
	VLua::RegisterGlobalVariable( "OBS_NONE", OBS_NONE );
	VLua::RegisterGlobalVariable( "OBS_CHASE_LOCKED", OBS_CHASE_LOCKED );
	VLua::RegisterGlobalVariable( "OBS_CHASE_FREE", OBS_CHASE_FREE );
	VLua::RegisterGlobalVariable( "OBS_ROAMING", OBS_ROAMING );
	VLua::RegisterGlobalVariable( "OBS_IN_EYE", OBS_IN_EYE );
	VLua::RegisterGlobalVariable( "OBS_MAP_FREE", OBS_MAP_FREE );
	VLua::RegisterGlobalVariable( "OBS_MAP_CHASE", OBS_MAP_CHASE );
	
	// Use Types
	VLua::RegisterGlobalVariable( "USE_OFF", 0 );
	VLua::RegisterGlobalVariable( "USE_ON", 1 );
	VLua::RegisterGlobalVariable( "USE_SET", 2 );
	VLua::RegisterGlobalVariable( "USE_TOGGLE", 3 );
	VLua::RegisterGlobalVariable( "USE_KILL", 4 );
	
	// Solid
	VLua::RegisterGlobalVariable( "SOLID_NOT", SOLID_NOT );
	VLua::RegisterGlobalVariable( "SOLID_TRIGGER", SOLID_TRIGGER );
	VLua::RegisterGlobalVariable( "SOLID_BBOX", SOLID_BBOX );
	VLua::RegisterGlobalVariable( "SOLID_SLIDEBOX", SOLID_SLIDEBOX );
	VLua::RegisterGlobalVariable( "SOLID_BSP", SOLID_BSP );
	
	// Deadflag
	VLua::RegisterGlobalVariable( "DEAD_NO", DEAD_NO );
	VLua::RegisterGlobalVariable( "DEAD_DYING", DEAD_DYING );
	VLua::RegisterGlobalVariable( "DEAD_DEAD", DEAD_DEAD );
	VLua::RegisterGlobalVariable( "DEAD_RESPAWNABLE", DEAD_RESPAWNABLE );
	VLua::RegisterGlobalVariable( "DEAD_DISCARDBODY", DEAD_DISCARDBODY );
	
	// Damage
	VLua::RegisterGlobalVariable( "DAMAGE_NO", DAMAGE_NO );
	VLua::RegisterGlobalVariable( "DAMAGE_YES", DAMAGE_YES );
	VLua::RegisterGlobalVariable( "DAMAGE_AIM", DAMAGE_AIM );
	
	// Entity Effects
	VLua::RegisterGlobalVariable( "EF_BRIGHTFIELD", EF_BRIGHTFIELD );
	VLua::RegisterGlobalVariable( "EF_MUZZLEFLASH", EF_MUZZLEFLASH );
	VLua::RegisterGlobalVariable( "EF_BRIGHTLIGHT", EF_BRIGHTLIGHT );
	VLua::RegisterGlobalVariable( "EF_DIMLIGHT", EF_DIMLIGHT );
	VLua::RegisterGlobalVariable( "EF_INVLIGHT", EF_INVLIGHT );
	VLua::RegisterGlobalVariable( "EF_NOINTERP", EF_NOINTERP );
	VLua::RegisterGlobalVariable( "EF_LIGHT", EF_LIGHT );
	VLua::RegisterGlobalVariable( "EF_NODRAW", EF_NODRAW );
	VLua::RegisterGlobalVariable( "EF_NOANIMTEXTURES", EF_NOANIMTEXTURES );
	VLua::RegisterGlobalVariable( "EF_FRAMEANIMTEXTURES", EF_FRAMEANIMTEXTURES );
	VLua::RegisterGlobalVariable( "EF_SPRITE_CUSTOM_VP", EF_SPRITE_CUSTOM_VP );
	
	// Render Types
	VLua::RegisterGlobalVariable<int>( "kRenderNormal", kRenderNormal );
	VLua::RegisterGlobalVariable<int>( "kRenderTransColor", kRenderTransColor );
	VLua::RegisterGlobalVariable<int>( "kRenderTransTexture", kRenderTransTexture );
	VLua::RegisterGlobalVariable<int>( "kRenderGlow", kRenderGlow );
	VLua::RegisterGlobalVariable<int>( "kRenderTransAlpha", kRenderTransAlpha );
	VLua::RegisterGlobalVariable<int>( "kRenderTransAdd", kRenderTransAdd );
	
	// Render Effects
	VLua::RegisterGlobalVariable<int>( "kRenderFxNone", kRenderFxNone );
	VLua::RegisterGlobalVariable<int>( "kRenderFxPulseSlow", kRenderFxPulseSlow );
	VLua::RegisterGlobalVariable<int>( "kRenderFxPulseFast", kRenderFxPulseFast );
	VLua::RegisterGlobalVariable<int>( "kRenderFxPulseSlowWide", kRenderFxPulseSlowWide );
	VLua::RegisterGlobalVariable<int>( "kRenderFxPulseFastWide", kRenderFxPulseFastWide );
	VLua::RegisterGlobalVariable<int>( "kRenderFxFadeSlow", kRenderFxFadeSlow );
	VLua::RegisterGlobalVariable<int>( "kRenderFxFadeFast", kRenderFxFadeFast );
	VLua::RegisterGlobalVariable<int>( "kRenderFxSolidSlow", kRenderFxSolidSlow );
	VLua::RegisterGlobalVariable<int>( "kRenderFxSolidFast", kRenderFxSolidFast );
	VLua::RegisterGlobalVariable<int>( "kRenderFxStrobeSlow", kRenderFxStrobeSlow );
	VLua::RegisterGlobalVariable<int>( "kRenderFxStrobeFast", kRenderFxStrobeFast );
	VLua::RegisterGlobalVariable<int>( "kRenderFxStrobeFaster", kRenderFxStrobeFaster );
	VLua::RegisterGlobalVariable<int>( "kRenderFxFlickerSlow", kRenderFxFlickerSlow );
	VLua::RegisterGlobalVariable<int>( "kRenderFxFlickerFast", kRenderFxFlickerFast );
	VLua::RegisterGlobalVariable<int>( "kRenderFxNoDissipation", kRenderFxNoDissipation );
	VLua::RegisterGlobalVariable<int>( "kRenderFxDistort", kRenderFxDistort );
	VLua::RegisterGlobalVariable<int>( "kRenderFxHologram", kRenderFxHologram );
	VLua::RegisterGlobalVariable<int>( "kRenderFxDeadPlayer", kRenderFxDeadPlayer );
	VLua::RegisterGlobalVariable<int>( "kRenderFxExplode", kRenderFxExplode );
	VLua::RegisterGlobalVariable<int>( "kRenderFxGlowShell", kRenderFxGlowShell );
	VLua::RegisterGlobalVariable<int>( "kRenderFxClampMinScale", kRenderFxClampMinScale );
	VLua::RegisterGlobalVariable<int>( "kRenderFxLightMultiplier", kRenderFxLightMultiplier );

	return 1;
}