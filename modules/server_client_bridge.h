#ifndef SERVER_CLIENT_BRIDGE_H
#define SERVER_CLIENT_BRIDGE_H

#ifdef _WIN32
#pragma once
#endif

#include <hl_sdk/engine/progdefs.h>

//-----------------------------------------------------------------------------
// SvenInt user message
//-----------------------------------------------------------------------------

#define SVC_SVENINT							( 146 ) // SvenInt user message
#define SVENINT_COMM_SETCVAR				( 0 )
#define SVENINT_COMM_EXECUTE				( 1 )
#define SVENINT_COMM_TIMER					( 2 )
#define SVENINT_COMM_TIMESCALE				( 3 )
#define SVENINT_COMM_DISPLAY_PLAYER_HULL	( 4 )
#define SVENINT_COMM_SCRIPTS				( 5 )

//-----------------------------------------------------------------------------
// Server-Client bridge
//-----------------------------------------------------------------------------

class CServerClientBridge
{
public:
	void Init( void );
	void Shutdown( void );

	void OnClientPutInServer( edict_t *pPlayer );
};

extern CServerClientBridge g_ServerClientBridge;

#endif // SERVER_CLIENT_BRIDGE_H