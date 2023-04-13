#ifndef SERVER_DLL_H
#define SERVER_DLL_H

#ifdef _WIN32
#pragma once
#endif

#include <hl_sdk/engine/progdefs.h>
#include <hl_sdk/engine/eiface.h>
#include <sys.h>

typedef qboolean (__cdecl *Host_IsServerActiveFn)(void);

//-----------------------------------------------------------------------------
// Server related globals
//-----------------------------------------------------------------------------

extern HMODULE g_hServerDLL;

extern globalvars_t *gpGlobals;

extern enginefuncs_t *g_pServerEngineFuncs;
extern DLL_FUNCTIONS *g_pServerFuncs;
extern NEW_DLL_FUNCTIONS *g_pNewServerFuncs;

extern Host_IsServerActiveFn Host_IsServerActive;

//-----------------------------------------------------------------------------
// Server functions
//-----------------------------------------------------------------------------

bool IsSurvivalModeEnabled( void );
bool EnableSurvivalMode( void );
bool DisableSurvivalMode( void );

//-----------------------------------------------------------------------------
// Initialize server's library
//-----------------------------------------------------------------------------

bool InitServerDLL();
void PostInitServerDLL();
void ShutdownServerDLL();

#endif // SERVER_DLL_H