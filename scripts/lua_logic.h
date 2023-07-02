#ifndef LUA_LOGIC_H
#define LUA_LOGIC_H

#ifdef _WIN32
#pragma once
#endif

#include "lua/lua.hpp"
#include "scripts.h"

#include <math/vector.h>
#include <math/mathlib.h>

#include <string>
#include <vector>

//-----------------------------------------------------------------------------
// Timers
//-----------------------------------------------------------------------------

typedef enum
{
	LUA_TYPE_NIL = 0,
	LUA_TYPE_BOOLEAN,
	LUA_TYPE_INTEGER,
	LUA_TYPE_NUMBER,
	LUA_TYPE_CSTRING,
	LUA_TYPE_CFUNCTION,
	LUA_TYPE_VECTOR,
	LUA_TYPE_REFERENCE
} LUA_TYPE;

#pragma warning(push)
#pragma warning(disable: 26495)
class ScriptTypeVariant
{
public:
	ScriptTypeVariant() { m_type = LUA_TYPE_NIL; }
	ScriptTypeVariant(bool value) { m_boolean = value; m_type = LUA_TYPE_BOOLEAN; }
	ScriptTypeVariant(lua_Integer value) { m_integer = value; m_type = LUA_TYPE_INTEGER; }
	ScriptTypeVariant(lua_Number value) { m_number = value; m_type = LUA_TYPE_NUMBER; }
	ScriptTypeVariant(const char *value) { m_cstring = strdup(value); m_type = LUA_TYPE_CSTRING; }
	ScriptTypeVariant(lua_CFunction value) { m_cfunction = value; m_type = LUA_TYPE_CFUNCTION; }
	//ScriptTypeVariant(Vector value) { m_vector = value; m_type = LUA_TYPE_VECTOR; }
	ScriptTypeVariant(scriptref_t value) { m_ref = value; m_type = LUA_TYPE_REFERENCE; }

	~ScriptTypeVariant()
	{
		//Free();
	}

	void operator=(bool value) { m_boolean = value; m_type = LUA_TYPE_BOOLEAN; }
	void operator=(lua_Integer value) { m_integer = value; m_type = LUA_TYPE_INTEGER; }
	void operator=(lua_Number value) { m_number = value; m_type = LUA_TYPE_NUMBER; }
	void operator=(const char *value) { m_cstring = strdup(value); m_type = LUA_TYPE_CSTRING; }
	void operator=(lua_CFunction value) { m_cfunction = value; m_type = LUA_TYPE_CFUNCTION; }
	//void operator=(Vector value) { m_vector = value; m_type = LUA_TYPE_VECTOR; }
	void operator=(scriptref_t value) { m_ref = value; m_type = LUA_TYPE_REFERENCE; }

	inline void Free()
	{
		if ( m_type == LUA_TYPE_CSTRING && m_cstring != NULL ) 
		{
			free((void *)m_cstring);
			m_cstring = NULL;
		}
	}

	union
	{
		bool m_boolean;
		const char *m_cstring;
		lua_Integer m_integer;
		lua_Number m_number;
		lua_CFunction m_cfunction;
		//Vector m_vector;
		scriptref_t m_ref;
	};

	int m_type;
};
#pragma warning(pop)

struct Lua_TimerContext
{
	lua_Integer id;
	double time;
	scriptref_t func_ref;
	std::vector<ScriptTypeVariant> args;
};

class CTimersHandler
{
	friend static int ScriptFunc_CreateTimer(lua_State *pLuaState);

public:
	CTimersHandler();

	void Frame(lua_State *pLuaState);

	bool RemoveTimer(lua_Integer id);
	void ClearTimers();

private:
	lua_Integer m_ID;
	std::vector<Lua_TimerContext> m_vTimers;
};

extern int luaopen_logic(lua_State *L);
extern CTimersHandler g_TimersHandler;

#endif // LUA_LOGIC_H