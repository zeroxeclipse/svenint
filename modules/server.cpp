#include "server.h"

#include "../patterns.h"

#include <dbg.h>
#include <ISvenModAPI.h>

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

HMODULE g_hServerDLL = NULL;

globalvars_t *gpGlobals = NULL;
globalvars_t **gpGlobals_ptr = NULL;

enginefuncs_t *g_pServerEngineFuncs = NULL;
DLL_FUNCTIONS *g_pServerFuncs = NULL;
NEW_DLL_FUNCTIONS *g_pNewServerFuncs = NULL;

Host_IsServerActiveFn Host_IsServerActive = NULL;

// stores dll funcs
static enginefuncs_t g_ServerEngineFuncs;
static DLL_FUNCTIONS g_ServerFuncs;
static NEW_DLL_FUNCTIONS g_NewServerFuncs;

//-----------------------------------------------------------------------------
// Initialize server's library
//-----------------------------------------------------------------------------

bool InitServerDLL()
{
	ud_t inst;

	g_hServerDLL = Sys_GetModuleHandle("server.dll");

	// Load server library
	if ( g_hServerDLL == NULL )
	{
		void *pfnSys_InitializeGameDLL = MemoryUtils()->FindPattern( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::Sys_InitializeGameDLL );

		if ( !pfnSys_InitializeGameDLL )
		{
			Warning("Couldn't find function \"Sys_InitializeGameDLL\"\n");
			return false;
		}
	
		( ( void (*)(void) )pfnSys_InitializeGameDLL )();

		if ( (g_hServerDLL = Sys_GetModuleHandle("server.dll")) == NULL )
		{
			Warning("Failed to load server's binary\n");
			return false;
		}

		Msg("[Sven Internal] Preloaded server binary\n");
	}

	// Get API functions
	int iNewDllFunctionsVersion = NEW_DLL_FUNCTIONS_VERSION;

	void *GiveFnptrsToDll = Sys_GetProcAddress(g_hServerDLL, "GiveFnptrsToDll");
	APIFUNCTION GetEntityAPI = (APIFUNCTION)Sys_GetProcAddress(g_hServerDLL, "GetEntityAPI");
	NEW_DLL_FUNCTIONS_FN GetNewDLLFunctions = (NEW_DLL_FUNCTIONS_FN)Sys_GetProcAddress(g_hServerDLL, "GetNewDLLFunctions");

	if ( GiveFnptrsToDll == NULL )
	{
		Warning("Failed to get function \"GiveFnptrsToDll\"\n");
		return false;
	}
	
	if ( GetEntityAPI == NULL )
	{
		Warning("Failed to get function \"GetEntityAPI\"\n");
		return false;
	}
	
	if ( GetNewDLLFunctions == NULL )
	{
		Warning("Failed to get function \"GetNewDLLFunctions\"\n");
		return false;
	}
	
	if ( !GetEntityAPI(&g_ServerFuncs, INTERFACE_VERSION) )
	{
		Warning("Failed to get DLL functions\n");
		return false;
	}
	
	if ( !GetNewDLLFunctions(&g_NewServerFuncs, &iNewDllFunctionsVersion) )
	{
		Warning("Failed to get new DLL functions\n");
		return false;
	}

	enginefuncs_t *pServerEngineFuncs = NULL;

	MemoryUtils()->InitDisasm(&inst, GiveFnptrsToDll, 32, 32);

	while ( MemoryUtils()->Disassemble(&inst) )
	{
		if ( inst.mnemonic == UD_Imov && inst.operand[0].type == UD_OP_REG && inst.operand[0].base == UD_R_EDI && inst.operand[1].type == UD_OP_IMM )
		{
			memcpy( &g_ServerEngineFuncs, pServerEngineFuncs = reinterpret_cast<enginefuncs_t *>(inst.operand[1].lval.udword), sizeof(enginefuncs_t) );
			break;
		}
	}

	if ( pServerEngineFuncs == NULL )
	{
		Warning("Failed to get server's engine functions\n");
		return false;
	}

	g_pServerEngineFuncs = &g_ServerEngineFuncs;
	g_pServerFuncs = &g_ServerFuncs;
	g_pNewServerFuncs = &g_NewServerFuncs;

	// Get Host_IsServerActive
	void *pfnCL_ClearState = MemoryUtils()->FindPattern( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::CL_ClearState );

	if ( !pfnCL_ClearState )
	{
		Warning("Failed to locate function \"Host_IsServerActive\"\n");
		return false;
	}

	MemoryUtils()->InitDisasm(&inst, pfnCL_ClearState, 32, 16);
	
	if ( MemoryUtils()->Disassemble(&inst) )
	{
		if (inst.mnemonic == UD_Icall)
		{
			Host_IsServerActive = (Host_IsServerActiveFn)MemoryUtils()->CalcAbsoluteAddress( pfnCL_ClearState );
		}
	}
	else
	{
		Warning("Couldn't locate function \"Host_IsServerActive\" #2\n");
		return false;
	}

	if ( !Host_IsServerActive )
	{
		Warning("Failed to get function \"Host_IsServerActive\"\n");
		return false;
	}

	// Get gpGlobals
	void *pgpGlobals = MemoryUtils()->FindPattern( g_hServerDLL, Patterns::Server::gpGlobals );

	if ( !pgpGlobals )
	{
		Warning("Couldn't locate \"gpGlobals\"\n");
		return false;
	}

	MemoryUtils()->InitDisasm(&inst, pgpGlobals, 32, 16);

	if ( MemoryUtils()->Disassemble(&inst) )
	{
		if (inst.mnemonic == UD_Imov && inst.operand[0].type == UD_OP_REG && inst.operand[0].base == UD_R_EAX && inst.operand[1].type == UD_OP_MEM)
		{
			gpGlobals_ptr = reinterpret_cast<globalvars_t **>(inst.operand[1].lval.udword);
		}
	}
	else
	{
		Warning("Couldn't locate \"gpGlobals\" #2\n");
		return false;
	}

	if ( !gpGlobals_ptr || !(gpGlobals = *gpGlobals_ptr) )
	{
		Warning("Failed to get \"gpGlobals\"\n");
		return false;
	}

	// Tertiary attack glitch
	extern void InitTertiaryAttackGlitch_Server(HMODULE hServerDLL);
	InitTertiaryAttackGlitch_Server( g_hServerDLL );

	return true;
}