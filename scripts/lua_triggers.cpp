#include "lua_triggers.h"
#include "lua_vector.h"
#include "scripts.h"

#include <dbg.h>
#include <convar.h>
#include <IUtils.h>
#include <IRender.h>
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
// CClientTriggerManager
//-----------------------------------------------------------------------------

CClientTriggerManager g_ClientTriggerManager;

CClientTriggerManager::CClientTriggerManager()
{
}

void CClientTriggerManager::Frame(lua_State *pLuaState)
{
	auto IsAABBIntersectingAABB = [](Vector &vecBoxMins1, Vector &vecBoxMaxs1, Vector &vecBoxMins2, Vector &vecBoxMaxs2) -> bool
	{
		return (vecBoxMins1.x <= vecBoxMaxs2.x && vecBoxMaxs1.x >= vecBoxMins2.x) &&
			(vecBoxMins1.y <= vecBoxMaxs2.y && vecBoxMaxs1.y >= vecBoxMins2.y) &&
			(vecBoxMins1.z <= vecBoxMaxs2.z && vecBoxMaxs1.z >= vecBoxMins2.z);
	};

	Vector vecOrigin;
	Vector vecMins, vecMaxs;

	VectorCopy( g_pPlayerMove->origin, vecOrigin );
	VectorCopy( g_pPlayerMove->origin, vecMins );
	VectorCopy( g_pPlayerMove->origin, vecMaxs );

	VectorAdd( vecMins, g_pPlayerMove->player_mins[g_pPlayerMove->usehull], vecMins );
	VectorAdd( vecMaxs, g_pPlayerMove->player_maxs[g_pPlayerMove->usehull], vecMaxs );

	for (size_t i = 0; i < m_vTriggers.size(); i++)
	{
		Lua_ClientTrigger &trigger = m_vTriggers[i];

		Vector vecTriggerMins = trigger.origin + trigger.mins;
		Vector vecTriggerMaxs = trigger.origin + trigger.maxs;

		if ( IsAABBIntersectingAABB(vecMins, vecMaxs, vecTriggerMins, vecTriggerMaxs) )
		{
			scriptref_t hFunction;

			if ( hFunction = g_ScriptVM.LookupFunction("OnTouchTrigger") )
			{
				lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, hFunction );
				lua_pushstring( pLuaState, trigger.name.c_str() );

				g_ScriptVM.ProtectedCall( pLuaState, 1, 0, 0 );
			}

			trigger.name.erase();
			m_vTriggers.erase(m_vTriggers.begin() + i);
			i--;
		}
	}
}

void CClientTriggerManager::AddTrigger(const char *pszName, const Vector &vecOrigin, const Vector &vecMins, const Vector &vecMaxs)
{
	Lua_ClientTrigger trigger;

	trigger.name = pszName;
	trigger.origin = vecOrigin;
	trigger.mins = vecMins;
	trigger.maxs = vecMaxs;

	m_vTriggers.push_back( trigger );
}

void CClientTriggerManager::ClearTriggers()
{
	m_vTriggers.clear();
}

//-----------------------------------------------------------------------------
// C to Lua
//-----------------------------------------------------------------------------

static int CreateTrigger(lua_State *pLuaState)
{
	const char *pszName = lua_tostring(pLuaState, 1);
	Vector *vecOrigin = lua_getvector(pLuaState, 2);
	Vector *vecMins = lua_getvector(pLuaState, 3);
	Vector *vecMaxs = lua_getvector(pLuaState, 4);

	g_ClientTriggerManager.AddTrigger(pszName, *vecOrigin, *vecMins, *vecMaxs);

	return 0;
}

static int ClearTriggers(lua_State *pLuaState)
{
	g_ClientTriggerManager.ClearTriggers();

	return 0;
}

//-----------------------------------------------------------------------------
// Init lib
//-----------------------------------------------------------------------------

LUALIB_API int luaopen_triggers(lua_State *pLuaState)
{
	lua_pushcfunction(pLuaState, CreateTrigger);
	lua_setglobal(pLuaState, "CreateTrigger");
	
	lua_pushcfunction(pLuaState, ClearTriggers);
	lua_setglobal(pLuaState, "ClearTriggers");

	return 1;
}