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

struct Lua_ClientTrigger
{
	std::string name;

	Vector origin;
	Vector mins;
	Vector maxs;
};

class CClientTriggerManager
{
public:
	CClientTriggerManager();

	void Frame(lua_State *pLuaState);
	void AddTrigger(const char *pszName, const Vector &vecOrigin, const Vector &vecMins, const Vector &vecMaxs);
	void ClearTriggers();

private:
	std::vector<Lua_ClientTrigger> m_vTriggers;
};

extern int luaopen_triggers(lua_State *L);
extern CClientTriggerManager g_ClientTriggerManager;

#endif // LUA_TRIGGERS_H