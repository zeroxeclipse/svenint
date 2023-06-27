#ifndef LUA_INPUT_MANAGER_H
#define LUA_INPUT_MANAGER_H

#ifdef _WIN32
#pragma once
#endif

#include "lua/lua.hpp"

extern int luaopen_inputmanager(lua_State *L);

#endif // LUA_INPUT_MANAGER_H