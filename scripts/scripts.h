#ifndef SCRIPTVM_H
#define SCRIPTVM_H

#ifdef _WIN32
#pragma once
#endif

#include "lua/lua.hpp"

#include <hl_sdk/engine/edict.h>
#include <hl_sdk/common/usercmd.h>
#include <client_state.h>

typedef int scriptref_t;

//-----------------------------------------------------------------------------
// Scripts Callbacks
//-----------------------------------------------------------------------------

class CScriptCallbacks
{
public:
	void OnGameFrame( client_state_t state, double frametime, bool bPostRunCmd );

	void OnFirstClientdataReceived(float flTime);

	void OnBeginLoading( void );
	void OnEndLoading( void );

	void OnDisconnect( void );
	void OnRestart( void );

	void OnEntityUse( edict_t *pEntityUseEdict, edict_t *pEntityEdict );
	void OnEntityTouch( edict_t *pEntityTouchEdict, edict_t *pEntityEdict );

	void OnClientPutInServer( edict_t *pPlayerEdict );

	void OnPlayerSpawn( edict_t *pSpawnSpotEdict, edict_t *pPlayerEdict );
	void OnSpecialSpawn( edict_t *pPlayerEdict );
	void OnPlayerUnstuck( edict_t *pPlayerEdict );
	void OnBeginPlayerRevive( edict_t *pPlayerEdict );
	void OnEndPlayerRevive( edict_t *pPlayerEdict );
	void OnClientKill( edict_t *pPlayerEdict );

	void OnServerSignal( int value );

	// Input Manager
	void OnPlayInput( const char *pszFilename, int frame, usercmd_t *cmd );
	void OnPlayEnd( const char *pszFilename, int frames );
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

	void		ResetStates( void );
	void		Frame( client_state_t state, double frametime, bool bPostRunCmd );

	lua_State	*GetVM( void );

	void		SetSearchPath( const char *pszSearchPath );

	bool		RunScript( const char *pszScript );
	bool		RunScriptFile( const char *pszFilename );

	scriptref_t LookupFunction( const char *pszFunction );
	void		ReleaseFunction( scriptref_t hFunction );

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