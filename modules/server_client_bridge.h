#ifndef SERVER_CLIENT_BRIDGE_H
#define SERVER_CLIENT_BRIDGE_H

#ifdef _WIN32
#pragma once
#endif

#define SVC_SVENINT 146 // SvenInt user message
#define SVENINT_COMM_SETCVAR 0
#define SVENINT_COMM_EXECUTE 1
#define SVENINT_COMM_TIMER 2
#define SVENINT_COMM_TIMESCALE 3

//-----------------------------------------------------------------------------
// Initialize server-client bridge
//-----------------------------------------------------------------------------

void InitServerClientBridge();

//-----------------------------------------------------------------------------
// Shutdown server-client bridge
//-----------------------------------------------------------------------------

void ShutdownServerClientBridge();

#endif // SERVER_CLIENT_BRIDGE_H