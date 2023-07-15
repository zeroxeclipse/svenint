#ifndef LUA_RANDOM_H
#define LUA_RANDOM_H

#ifdef _WIN32
#pragma once
#endif

#include "lua/lua.hpp"

extern int luaopen_random(lua_State *L);

#endif // LUA_RANDOM_H