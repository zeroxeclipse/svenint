#ifndef SCRIPTVM_H
#define SCRIPTVM_H

#ifdef _WIN32
#pragma once
#endif

#include "lua/lua.hpp"

#include <client_state.h>

//-----------------------------------------------------------------------------
// Scripts Callbacks
//-----------------------------------------------------------------------------

class CScriptCallbacks
{
public:
	void OnFirstClientdataReceived(float flTime);

	void OnBeginLoading( void );
	void OnEndLoading( void );

	void OnDisconnect( void );
};

extern CScriptCallbacks g_ScriptCallbacks;

//-----------------------------------------------------------------------------
// Scripts Virtual Machine
//-----------------------------------------------------------------------------

class CScriptVM
{
	friend CScriptCallbacks;
	friend class CTimersHandler;
	friend class CClientTriggerManager;

public:
	bool		Init( void );
	void		Shutdown( void );

	void		Frame( client_state_t state, double frametime, bool bPostRunCmd );

	lua_State	*GetVM( void );

	void		SetSearchPath( const char *pszSearchPath );

	bool		RunScript( const char *pszScript );
	bool		RunScriptFile( const char *pszFilename );

	bool		LookupFunction( const char *pszFunction );

private:
	void		PrintError(void);
	void		DumpStack(void);

private:
	lua_State *m_pLuaState;
};

extern CScriptVM g_ScriptVM;
extern CScriptVM *g_pScriptVM;

#endif // SCRIPTVM_H