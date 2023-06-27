#ifndef LUA_PLAYERMOVE_H
#define LUA_PLAYERMOVE_H

#ifdef _WIN32
#pragma once
#endif

#include <hl_sdk/pm_shared/pm_defs.h>

#include "lua/lua.hpp"

extern int luaopen_playermove( lua_State *pLuaState );
extern bool lua_isplayermove( lua_State *pLuaState, int i );
extern playermove_t *lua_getplayermove( lua_State *pLuaState, int i );
extern void lua_pushplayermove( lua_State *pLuaState, playermove_t *pPlayerMove );

#endif // LUA_PLAYERMOVE_H