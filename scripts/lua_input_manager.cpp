#include "lua_input_manager.h"

#include "../features/input_manager.h"

//-----------------------------------------------------------------------------
// Script funcs
//-----------------------------------------------------------------------------

static int ScriptFunc_Record(lua_State *pLuaState)
{
	const char *pszFilename = lua_tostring( pLuaState, 1 );

	lua_pushboolean( pLuaState, g_InputManager.Record( pszFilename ) );

	return 1;
}

static int ScriptFunc_Playback(lua_State *pLuaState)
{
	const char *pszFilename = lua_tostring( pLuaState, 1 );

	lua_pushboolean( pLuaState, g_InputManager.Playback( pszFilename ) );

	return 1;
}

static int ScriptFunc_Split(lua_State *pLuaState)
{
	lua_pushboolean( pLuaState, g_InputManager.Split() );

	return 1;
}

static int ScriptFunc_Goto(lua_State *pLuaState)
{
	int iFrame = (int)lua_tointeger( pLuaState, 1 );

	g_InputManager.Goto( iFrame );

	return 0;
}

static int ScriptFunc_Forward(lua_State *pLuaState)
{
	int iFrames = (int)lua_tointeger( pLuaState, 1 );

	g_InputManager.Forward( iFrames );

	return 0;
}

static int ScriptFunc_Backward(lua_State *pLuaState)
{
	int iFrames = (int)lua_tointeger( pLuaState, 1 );

	g_InputManager.Backward( iFrames );

	return 0;
}

static int ScriptFunc_Stop(lua_State *pLuaState)
{
	lua_pushboolean( pLuaState, g_InputManager.Stop() );

	return 1;
}

static int ScriptFunc_RecordCommand(lua_State *pLuaState)
{
	const char *pszCommand = lua_tostring( pLuaState, 1 );

	g_InputManager.RecordCommand( pszCommand );

	return 0;
}

static int ScriptFunc_IsInAction(lua_State *pLuaState)
{
	lua_pushboolean( pLuaState, g_InputManager.IsInAction() );

	return 1;
}

static int ScriptFunc_IsRecording(lua_State *pLuaState)
{
	lua_pushboolean( pLuaState, g_InputManager.IsRecording() );

	return 1;
}

static int ScriptFunc_IsPlayingback(lua_State *pLuaState)
{
	lua_pushboolean( pLuaState, g_InputManager.IsPlayingback() );

	return 1;
}

static int ScriptFunc_GetCurrentFrame(lua_State *pLuaState)
{
	lua_pushinteger( pLuaState, (lua_Integer)g_InputManager.GetCurrentFrame() );

	return 1;
}

static const luaL_Reg IM_Registrations[] =
{
	{ "Record",				ScriptFunc_Record },
	{ "Playback",			ScriptFunc_Playback },
	{ "Split",				ScriptFunc_Split },
	{ "Stop",				ScriptFunc_Stop },
	{ "Goto",				ScriptFunc_Goto },
	{ "Forward",			ScriptFunc_Forward },
	{ "Backward",			ScriptFunc_Backward },
	{ "RecordCommand",		ScriptFunc_RecordCommand },
	{ "IsInAction",			ScriptFunc_IsInAction },
	{ "IsRecording",		ScriptFunc_IsRecording },
	{ "IsPlayingback",		ScriptFunc_IsPlayingback },
	{ "GetCurrentFrame",	ScriptFunc_GetCurrentFrame },
	{ NULL,					NULL }
};

//-----------------------------------------------------------------------------
// Initialize library
//-----------------------------------------------------------------------------

LUALIB_API int luaopen_inputmanager(lua_State *pLuaState)
{
	lua_newtable(pLuaState);
	luaL_setfuncs(pLuaState, IM_Registrations, NULL);
	lua_setglobal(pLuaState, "InputManager");

	return 1;
}