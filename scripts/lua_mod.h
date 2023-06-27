#ifndef LUA_MOD_H
#define LUA_MOD_H

#ifdef _WIN32
#pragma once
#endif

#include "lua/lua.hpp"

extern void lua_setcurrentmap( lua_State *pLuaState );
extern int luaopen_mod(lua_State *L);

#endif // LUA_MOD_H