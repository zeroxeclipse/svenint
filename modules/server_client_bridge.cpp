#include "server_client_bridge.h"
#include "server.h"

#include <hl_sdk/common/protocol.h>

#include <ISvenModAPI.h>
#include <IDetoursAPI.h>
#include <messagebuffer.h>
#include <dbg.h>

#include "../features/speedrun_tools.h"
#include "../game/utils.h"

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

DECLARE_HOOK(void, __cdecl, ClientPutInServer, edict_t *);
DetourHandle_t hClientPutInServer = DETOUR_INVALID_HANDLE;

//-----------------------------------------------------------------------------
// User message hook
//-----------------------------------------------------------------------------

static int UserMsgHook_SvenInt(const char *pszUserMsg, int iSize, void *pBuffer)
{
    CMessageBuffer message(pBuffer, iSize, true);

    int type = message.ReadByte();

    if ( type == SVENINT_COMM_SETCVAR )
    {
        cvar_t *pCvar;
        const char *pszCvar = message.ReadString();

        if ( (pCvar = CVar()->FindCvar(pszCvar)) != NULL )
        {
            const char *pszValue = message.ReadString();
            CVar()->SetValue( pCvar, pszValue );
        }
    }
    else if ( type == SVENINT_COMM_EXECUTE )
    {
        const char *pszCommand = message.ReadString();
        g_pEngineFuncs->ClientCmd( pszCommand );
    }
    else if ( type == SVENINT_COMM_TIMER )
    {
        float time = message.ReadCoord();
        g_SpeedrunTools.ShowTimer( time, false );
    }
    else if ( type == SVENINT_COMM_TIMESCALE )
    {
        bool notify = !!message.ReadByte();

        float framerate = Long32ToFloat( message.ReadLong() );
        float fpsmax = Long32ToFloat( message.ReadLong() );
        float min_frametime = Long32ToFloat( message.ReadLong() );

        if ( !g_pDemoAPI->IsPlayingback() && !Host_IsServerActive() )
        {
            g_SpeedrunTools.SetTimescale_Comm( notify, framerate, fpsmax, min_frametime );
        }
    }
    else if ( type == SVENINT_COMM_DISPLAY_PLAYER_HULL )
    {
        int client, dead;
        Vector vecOrigin, vecMins, vecMaxs;

        client = message.ReadByte();
        dead = message.ReadByte();

        vecOrigin.x = Long32ToFloat( message.ReadLong() );
        vecOrigin.y = Long32ToFloat( message.ReadLong() );
        vecOrigin.z = Long32ToFloat( message.ReadLong() );
        
        vecMins.x = Long32ToFloat( message.ReadLong() );
        vecMins.y = Long32ToFloat( message.ReadLong() );
        vecMins.z = Long32ToFloat( message.ReadLong() );
        
        vecMaxs.x = Long32ToFloat( message.ReadLong() );
        vecMaxs.y = Long32ToFloat( message.ReadLong() );
        vecMaxs.z = Long32ToFloat( message.ReadLong() );

        if ( !g_pDemoAPI->IsPlayingback() )
        {
            g_SpeedrunTools.DrawPlayerHull_Comm( client, dead, vecOrigin, vecMins, vecMaxs );
        }
    }

	return 1;
}

//-----------------------------------------------------------------------------
// ClientPutInServer hook
//-----------------------------------------------------------------------------

DECLARE_FUNC(void, __cdecl, HOOKED_ClientPutInServer, edict_t *pEntity)
{
    ORIG_ClientPutInServer(pEntity);

    g_pServerEngineFuncs->pfnMessageBegin(MSG_ONE, SVC_NEWUSERMSG, NULL, pEntity);
        g_pServerEngineFuncs->pfnWriteByte(SVC_SVENINT);
        g_pServerEngineFuncs->pfnWriteByte(255);
        g_pServerEngineFuncs->pfnWriteLong(0x6E657653); // nevS
        g_pServerEngineFuncs->pfnWriteLong(0x00746E49); // tnI
        g_pServerEngineFuncs->pfnWriteLong(0x0);
        g_pServerEngineFuncs->pfnWriteLong(0x0);
    g_pServerEngineFuncs->pfnMessageEnd();

    g_SpeedrunTools.SendTimescale(pEntity);
}

//-----------------------------------------------------------------------------
// Initialize server-client bridge
//-----------------------------------------------------------------------------

void InitServerClientBridge()
{
	if ( g_pEngineFuncs->HookUserMsg("SvenInt", UserMsgHook_SvenInt) != 0 )
    {
        usermsg_t *pUserMsg = const_cast<usermsg_t *>( Utils()->FindUserMessage("SvenInt") );

        if ( pUserMsg != NULL )
        {
            pUserMsg->function = UserMsgHook_SvenInt;
        }
    }

    hClientPutInServer = DetoursAPI()->DetourFunction( g_pServerFuncs->pfnClientPutInServer, HOOKED_ClientPutInServer, GET_FUNC_PTR(ORIG_ClientPutInServer) );
}

//-----------------------------------------------------------------------------
// Shutdown server-client bridge
//-----------------------------------------------------------------------------

void ShutdownServerClientBridge()
{
    //usermsg_t *pUserMsg = const_cast<usermsg_t *>( Utils()->FindUserMessage("ScreenFade") );

    //if ( pUserMsg != NULL )
    //{
    //    bool bFound = false;
    //    usermsg_t *pPrev = NULL;

    //    while ( pUserMsg )
    //    {
    //        if ( !stricmp(pUserMsg->name, "SvenInt") )
    //        {
    //            if ( pPrev != NULL )
    //            {
    //                pPrev->next = pUserMsg->next;
    //            }

    //            free( (void *)pUserMsg );

    //            bFound = true;
    //            break;
    //        }

    //        pPrev = pUserMsg;
    //        pUserMsg = pUserMsg->next;
    //    }
    //}

    usermsg_t *pUserMsg = const_cast<usermsg_t *>( Utils()->FindUserMessage("SvenInt") );

    if ( pUserMsg != NULL )
    {
        pUserMsg->function = NULL;
    }

    DetoursAPI()->RemoveDetour( hClientPutInServer );
}