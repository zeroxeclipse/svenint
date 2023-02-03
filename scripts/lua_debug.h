#ifndef LUA_DEBUG_H
#define LUA_DEBUG_H

#ifdef _WIN32
#pragma once
#endif

#include "lua/lua.hpp"

extern int luaopen_print(lua_State *L);

#endif // LUA_DEBUG_H