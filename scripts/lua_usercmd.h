#ifndef LUA_USERCMD_H
#define LUA_USERCMD_H

#ifdef _WIN32
#pragma once
#endif

#include <hl_sdk/common/usercmd.h>

#include "lua/lua.hpp"

extern int luaopen_usercmd( lua_State *pLuaState );
extern bool lua_isusercmd( lua_State *pLuaState, int i );
extern usercmd_t *lua_getusercmd( lua_State *pLuaState, int i );
extern void lua_pushusercmd( lua_State *pLuaState, usercmd_t *cmd );

#endif // LUA_USERCMD_H