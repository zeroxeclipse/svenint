#ifndef LUA_TRIGGERS_H
#define LUA_TRIGGERS_H

#ifdef _WIN32
#pragma once
#endif

#include "lua/lua.hpp"

#include <math/mathlib.h>

#include <string>
#include <vector>

//-----------------------------------------------------------------------------
// Triggers
//-----------------------------------------------------------------------------

struct Lua_Trigger
{
	std::string name;

	Vector origin;
	Vector mins;
	Vector maxs;
};

class CLuaTriggerManager
{
public:
	CLuaTriggerManager();

	void Frame(lua_State *pLuaState);

	void AddTrigger(const char *pszName, const Vector &vecOrigin, const Vector &vecMins, const Vector &vecMaxs);
	void AddServerTrigger(const char *pszName, const Vector &vecOrigin, const Vector &vecMins, const Vector &vecMaxs);

	void ClearTriggers();
	void ClearServerTriggers();

private:
	void TriggersThink( lua_State *pLuaState );
	void ServerTriggersThink( lua_State *pLuaState );

private:
	std::vector<Lua_Trigger> m_vTriggers;
	std::vector<Lua_Trigger> m_vServerTriggers;
};

extern int luaopen_triggers(lua_State *L);
extern CLuaTriggerManager g_LuaTriggerManager;

#endif // LUA_TRIGGERS_H