#ifndef SCRIPTVM_H
#define SCRIPTVM_H

#ifdef _WIN32
#pragma once
#endif

#include "lua/lua.hpp"

#include <hl_sdk/engine/edict.h>
#include <client_state.h>

typedef int scriptref_t;

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
	void OnRestart( void );

	void OnPlayerSpawn( edict_t *pSpawnSpotEdict, edict_t *pPlayerEdict );
	void OnClientKill( edict_t *pPlayerEdict );
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

	scriptref_t LookupFunction( const char *pszFunction );
	void		ProtectedCall( lua_State *pLuaState, int args, int results, int errfunc );

private:
	void		PrintError(void);
	void		DumpStack(void);

private:
	lua_State *m_pLuaState;
};

extern CScriptVM g_ScriptVM;
extern CScriptVM *g_pScriptVM;

#endif // SCRIPTVM_H