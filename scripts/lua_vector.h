#ifndef LUA_VECTOR_H
#define LUA_VECTOR_H

#ifdef _WIN32
#pragma once
#endif

#include <math/vector.h>
#include "lua/lua.hpp"

extern int luaopen_vector(lua_State *L);
extern bool lua_isvector(lua_State *pLuaState, int i);
extern Vector *lua_getvector(lua_State *L, int i);
extern Vector *lua_newvector(lua_State *L, const Vector *Value);

#endif // LUA_VECTOR_H