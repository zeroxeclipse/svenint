#ifndef SERVER_DLL_H
#define SERVER_DLL_H

#ifdef _WIN32
#pragma once
#endif

#include <IDetoursAPI.h>

#include <hl_sdk/engine/edict.h>
#include <hl_sdk/engine/progdefs.h>
#include <hl_sdk/engine/eiface.h>
#include <sys.h>

//-----------------------------------------------------------------------------
// Signatures
//-----------------------------------------------------------------------------

FUNC_SIGNATURE( qboolean, __cdecl, Host_IsServerActiveFn, void );

//-----------------------------------------------------------------------------
// Server related globals
//-----------------------------------------------------------------------------

extern HMODULE g_hServerDLL;

extern globalvars_t *gpGlobals;

extern enginefuncs_t *g_pServerEngineFuncs;
extern DLL_FUNCTIONS *g_pServerFuncs;
extern NEW_DLL_FUNCTIONS *g_pNewServerFuncs;

//-----------------------------------------------------------------------------
// Server functions
//-----------------------------------------------------------------------------

extern Host_IsServerActiveFn Host_IsServerActive;

bool IsSurvivalModeEnabled( void );
bool EnableSurvivalMode( void );
bool DisableSurvivalMode( void );

FORCEINLINE bool FNullEnt( edict_t *pEntity )
{
	return pEntity == NULL || g_pServerEngineFuncs->pfnEntOffsetOfPEntity( pEntity ) == 0;
}

FORCEINLINE bool IsValidEntity( edict_t *pEntity )
{
	if ( pEntity == NULL || pEntity->free || pEntity->pvPrivateData == NULL || ( pEntity->v.flags & FL_KILLME ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Server's library module
//-----------------------------------------------------------------------------

class CServerModule
{
public:
	CServerModule();

	bool Init( void );
	void PostInit( void );
	void Shutdown( void );

	inline HMODULE ModuleHandle( void ) const { return m_hServerDLL; };

private:
	HMODULE m_hServerDLL;

	void *m_pCBasePlayerVMT;
	void *m_pfnPlayerSpawns;
	void *m_pfnFixPlayerStuck;
	void *m_pfnFireTargets;
	void *m_pfnCopyPEntityVars;

	DetourHandle_t m_hUse;
	DetourHandle_t m_hTouch;
	DetourHandle_t m_hPlayerSpawns;
	DetourHandle_t m_hFixPlayerStuck;
	DetourHandle_t m_hClientKill;
	DetourHandle_t m_hClientPutInServer;
	DetourHandle_t m_hClientCommand;
	DetourHandle_t m_hCBasePlayer__SpecialSpawn;
	DetourHandle_t m_hCBasePlayer__BeginRevive;
	DetourHandle_t m_hCBasePlayer__EndRevive;
	DetourHandle_t m_hFireTargets;
	DetourHandle_t m_hCopyPEntityVars;
};

extern CServerModule g_ServerModule;

#endif // SERVER_DLL_H