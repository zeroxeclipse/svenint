#include "lua_triggers.h"
#include "lua_vector.h"
#include "lua_entity_dictionary.h"
#include "scripts.h"

#include "../game/utils.h"
#include "../modules/server.h"

#include <dbg.h>
#include <convar.h>
#include <IUtils.h>
#include <IRender.h>
#include <hl_sdk/cl_dll/cl_dll.h>
#include <hl_sdk/pm_shared/pm_shared.h>

//-----------------------------------------------------------------------------
// ConCommands
//-----------------------------------------------------------------------------

static bool s_bFirstPointSet = false;
static Vector s_bFirstPoint;

CON_COMMAND(sc_set_trigger_point, "Sets a point of trigger")
{
	if ( !s_bFirstPointSet )
	{
		s_bFirstPoint = g_pPlayerMove->origin + g_pPlayerMove->view_ofs;
		s_bFirstPointSet = true;

		Msg("First trigger point has ben set\n");
		Utils()->PrintChatText("First trigger point has been set\n");
	}
	else
	{
		Vector vecMins, vecMaxs;

		Vector v1 = s_bFirstPoint;
		Vector v2 = g_pPlayerMove->origin + g_pPlayerMove->view_ofs;
		
		Vector vecPos = v1;
		float xDiff = v2.x - v1.x;
		float yDiff = v2.y - v1.y;
		float zDiff = v2.z - v1.z;

		if (xDiff < 0 && yDiff < 0)
		{
			xDiff *= -1;
			yDiff *= -1;
			vecPos.x -= xDiff;
			vecPos.y -= yDiff;
		}
		else if (xDiff > 0 && yDiff < 0)
		{
			yDiff *= -1;
			vecPos.y -= yDiff;
		}
		else if (xDiff < 0 && yDiff > 0)
		{
			xDiff *= -1;
			vecPos.x -= xDiff;
		}

		if (zDiff > 0)
		{
			vecMins.Zero();

			vecMaxs.x = xDiff;
			vecMaxs.y = yDiff;
			vecMaxs.z = zDiff;

			ConColorMsg({ 255, 255, 0, 255 }, "Dump: CreateTrigger(\"trigger\", Vector(%.3f, %.3f, %.3f), Vector(0, 0, 0), Vector(%.3f, %.3f, %.3f));\n", VectorExpand(vecPos), xDiff, yDiff, zDiff);
		}
		else
		{
			vecMins.x = 0.f;
			vecMins.y = 0.f;
			vecMins.z = zDiff;

			vecMaxs.x = xDiff;
			vecMaxs.y = yDiff;
			vecMaxs.z = 0.f;

			ConColorMsg({255, 255, 0, 255}, "Dump: CreateTrigger(\"trigger\", Vector(%.3f, %.3f, %.3f), Vector(0, 0, %.3f), Vector(%.3f, %.3f, 0));\n", VectorExpand(vecPos), zDiff, xDiff, yDiff);
		}

		Render()->DrawBox(vecPos, vecMins, vecMaxs, { 255, 255, 0, 100 }, 10.f);

		s_bFirstPointSet = false;
	}
}

//-----------------------------------------------------------------------------
// CLuaTriggerManager
//-----------------------------------------------------------------------------

CLuaTriggerManager g_LuaTriggerManager;

CLuaTriggerManager::CLuaTriggerManager()
{
}

void CLuaTriggerManager::Frame(lua_State *pLuaState)
{
	TriggersThink( pLuaState );

	if ( Host_IsServerActive() )
		ServerTriggersThink( pLuaState );
}

void CLuaTriggerManager::AddTrigger(const char *pszName, const Vector &vecOrigin, const Vector &vecMins, const Vector &vecMaxs)
{
	Lua_Trigger trigger;

	trigger.name = pszName;
	trigger.origin = vecOrigin;
	trigger.mins = vecMins;
	trigger.maxs = vecMaxs;

	m_vTriggers.push_back( trigger );
}

void CLuaTriggerManager::AddServerTrigger(const char *pszName, const Vector &vecOrigin, const Vector &vecMins, const Vector &vecMaxs)
{
	Lua_Trigger trigger;

	trigger.name = pszName;
	trigger.origin = vecOrigin;
	trigger.mins = vecMins;
	trigger.maxs = vecMaxs;

	m_vServerTriggers.push_back( trigger );
}

void CLuaTriggerManager::ClearTriggers()
{
	m_vTriggers.clear();
}

void CLuaTriggerManager::ClearServerTriggers()
{
	m_vServerTriggers.clear();
}

//-----------------------------------------------------------------------------
// Think
//-----------------------------------------------------------------------------

void CLuaTriggerManager::TriggersThink( lua_State *pLuaState )
{
	scriptref_t hCallbackFunction;

	Vector vecOrigin;
	Vector vecMins, vecMaxs;

	VectorCopy( g_pPlayerMove->origin, vecOrigin );
	VectorCopy( g_pPlayerMove->origin, vecMins );
	VectorCopy( g_pPlayerMove->origin, vecMaxs );

	VectorAdd( vecMins, ( g_pPlayerMove->flags & FL_DUCKING ) ? VEC_DUCK_HULL_MIN : VEC_HULL_MIN, vecMins );
	VectorAdd( vecMaxs, ( g_pPlayerMove->flags & FL_DUCKING ) ? VEC_DUCK_HULL_MAX : VEC_HULL_MAX, vecMaxs );

	//VectorAdd( vecMins, g_pPlayerMove->player_mins[ g_pPlayerMove->usehull ], vecMins );
	//VectorAdd( vecMaxs, g_pPlayerMove->player_maxs[ g_pPlayerMove->usehull ], vecMaxs );

	hCallbackFunction = g_ScriptVM.LookupFunction( "OnTouchTrigger" );

	for ( size_t i = 0; i < m_vTriggers.size(); i++ )
	{
		Lua_Trigger &trigger = m_vTriggers[ i ];

		Vector vecTriggerMins = trigger.origin + trigger.mins;
		Vector vecTriggerMaxs = trigger.origin + trigger.maxs;

		if ( UTIL_IsAABBIntersectingAABB( vecMins, vecMaxs, vecTriggerMins, vecTriggerMaxs ) )
		{
			if ( hCallbackFunction )
			{
				lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, hCallbackFunction );
				lua_pushstring( pLuaState, trigger.name.c_str() );

				g_ScriptVM.ProtectedCall( pLuaState, 1, 0, 0 );
			}

			trigger.name.erase();
			m_vTriggers.erase( m_vTriggers.begin() + i );
			i--;
		}
	}

	if ( hCallbackFunction )
	{
		g_ScriptVM.ReleaseFunction( hCallbackFunction );
	}
}

void CLuaTriggerManager::ServerTriggersThink( lua_State *pLuaState )
{
	scriptref_t hCallbackFunction;

	Vector vecOrigin;
	Vector vecMins, vecMaxs;

	// Nothing to do here when we don't have a callback function
	if ( m_vServerTriggers.size() == 0 || !( hCallbackFunction = g_ScriptVM.LookupFunction( "OnTouchServerTrigger" ) ) )
		return;

	for ( size_t i = 0; i < m_vServerTriggers.size(); i++ )
	{
		Lua_Trigger &trigger = m_vServerTriggers[ i ];

		Vector vecTriggerMins = trigger.origin + trigger.mins;
		Vector vecTriggerMaxs = trigger.origin + trigger.maxs;

		for ( int j = 1; j <= gpGlobals->maxClients; j++ )
		{
			edict_t *pPlayer = g_pServerEngineFuncs->pfnPEntityOfEntIndex( j );

			if ( !IsValidEntity( pPlayer ) )
				continue;

			VectorCopy( pPlayer->v.origin, vecOrigin );
			VectorCopy( pPlayer->v.origin, vecMins );
			VectorCopy( pPlayer->v.origin, vecMaxs );

			VectorAdd( vecMins, pPlayer->v.mins, vecMins );
			VectorAdd( vecMaxs, pPlayer->v.maxs, vecMaxs );

			if ( UTIL_IsAABBIntersectingAABB( vecMins, vecMaxs, vecTriggerMins, vecTriggerMaxs ) )
			{
				lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, hCallbackFunction );

				lua_pushedict( pLuaState, pPlayer );
				lua_pushstring( pLuaState, trigger.name.c_str() );

				g_ScriptVM.ProtectedCall( pLuaState, 2, 1, 0 );

				bool bRemoveTrigger = lua_toboolean( pLuaState, -1 );

				if ( bRemoveTrigger )
				{
					trigger.name.erase();

					m_vServerTriggers.erase( m_vServerTriggers.begin() + i );
					i--;

					break;
				}
			}
		}
	}

	g_ScriptVM.ReleaseFunction( hCallbackFunction );
}

//-----------------------------------------------------------------------------
// C to Lua
//-----------------------------------------------------------------------------

static int ScriptFunc_CreateTrigger(lua_State *pLuaState)
{
	const char *pszName = lua_tostring(pLuaState, 1);
	Vector *vecOrigin = lua_getvector(pLuaState, 2);
	Vector *vecMins = lua_getvector(pLuaState, 3);
	Vector *vecMaxs = lua_getvector(pLuaState, 4);

	g_LuaTriggerManager.AddTrigger(pszName, *vecOrigin, *vecMins, *vecMaxs);

	return 0;
}

static int ScriptFunc_CreateServerTrigger(lua_State *pLuaState)
{
	const char *pszName = lua_tostring(pLuaState, 1);
	Vector *vecOrigin = lua_getvector(pLuaState, 2);
	Vector *vecMins = lua_getvector(pLuaState, 3);
	Vector *vecMaxs = lua_getvector(pLuaState, 4);

	g_LuaTriggerManager.AddServerTrigger(pszName, *vecOrigin, *vecMins, *vecMaxs);

	return 0;
}

static int ScriptFunc_ClearTriggers(lua_State *pLuaState)
{
	g_LuaTriggerManager.ClearTriggers();

	return 0;
}

static int ScriptFunc_ClearServerTriggers(lua_State *pLuaState)
{
	g_LuaTriggerManager.ClearServerTriggers();

	return 0;
}

//-----------------------------------------------------------------------------
// Init lib
//-----------------------------------------------------------------------------

LUALIB_API int luaopen_triggers(lua_State *pLuaState)
{
	lua_pushcfunction(pLuaState, ScriptFunc_CreateTrigger);
	lua_setglobal(pLuaState, "CreateTrigger");
	
	lua_pushcfunction(pLuaState, ScriptFunc_CreateServerTrigger);
	lua_setglobal(pLuaState, "CreateServerTrigger");
	
	lua_pushcfunction(pLuaState, ScriptFunc_ClearTriggers);
	lua_setglobal(pLuaState, "ClearTriggers");
	
	lua_pushcfunction(pLuaState, ScriptFunc_ClearServerTriggers);
	lua_setglobal(pLuaState, "ClearServerTriggers");

	return 1;
}