#ifndef LUA_CVAR_H
#define LUA_CVAR_H

#ifdef _WIN32
#pragma once
#endif

#include "lua/lua.hpp"

extern int luaopen_cvar(lua_State *L);

#endif // LUA_CVAR_H