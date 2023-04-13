#ifndef LUA_EDICT_H
#define LUA_EDICT_H

#ifdef _WIN32
#pragma once
#endif

#include <hl_sdk/engine/edict.h>

#include "lua/lua.hpp"

extern int luaopen_edict( lua_State *pLuaState );
extern bool lua_isedict( lua_State *pLuaState, int i );
extern edict_t *lua_getedict( lua_State *pLuaState, int i );
extern void lua_pushedict( lua_State *pLuaState, edict_t *pEdict );

#endif // LUA_EDICT_H