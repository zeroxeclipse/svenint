#ifndef LUA_ENTVARS_H
#define LUA_ENTVARS_H

#ifdef _WIN32
#pragma once
#endif

#include <hl_sdk/engine/progdefs.h>

#include "lua/lua.hpp"

extern int luaopen_entvars( lua_State *pLuaState );
extern bool lua_isentvars( lua_State *pLuaState, int i );
extern entvars_t *lua_getentvars( lua_State *pLuaState, int i );
extern void lua_pushentvars( lua_State *pLuaState, entvars_t *pEntityVars );

#endif // LUA_ENTVARS_H