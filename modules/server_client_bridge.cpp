#include "server_client_bridge.h"
#include "server.h"

#include <hl_sdk/common/protocol.h>

#include <ISvenModAPI.h>
#include <IDetoursAPI.h>
#include <messagebuffer.h>
#include <dbg.h>

#include "../features/speedrun_tools.h"
#include "../scripts/scripts.h"
#include "../game/utils.h"

extern bool g_bPlayingbackDemo;

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

CServerClientBridge g_ServerClientBridge;

//-----------------------------------------------------------------------------
// User message hook
//-----------------------------------------------------------------------------

static int UserMsgHook_SvenInt(const char *pszUserMsg, int iSize, void *pBuffer)
{
    CMessageBuffer message( pBuffer, iSize, true );

    int type = message.ReadByte();

    if ( type == SVENINT_COMM_SETCVAR )
    {
        cvar_t *pCvar;
        const char *pszCvar = message.ReadString();

        if ( ( pCvar = CVar()->FindCvar( pszCvar ) ) != NULL )
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

        if ( !g_bPlayingbackDemo && !Host_IsServerActive() )
        {
            g_SpeedrunTools.SetTimescale_Comm( notify, framerate, fpsmax, min_frametime );
        }
    }
    else if ( type == SVENINT_COMM_DISPLAY_PLAYER_HULL )
    {
        struct
        {
            unsigned char client : 6;
            unsigned char dead : 1;
            unsigned char duck : 1;
        } displayInfo;

        Vector vecOrigin;

        *(unsigned char *)&displayInfo = message.ReadByte();

        vecOrigin.x = Long32ToFloat( message.ReadLong() );
        vecOrigin.y = Long32ToFloat( message.ReadLong() );
        vecOrigin.z = Long32ToFloat( message.ReadLong() );

        if ( !g_bPlayingbackDemo )
        {
            g_SpeedrunTools.DrawPlayerHull_Comm( displayInfo.client, displayInfo.dead, vecOrigin, !!displayInfo.duck );
        }
    }
    else if ( type == SVENINT_COMM_SCRIPTS )
    {
        int scriptMsgType = message.ReadByte();

        if ( !g_bPlayingbackDemo )
        {
            if ( scriptMsgType == 0 ) // signal from the server
            {
                g_ScriptCallbacks.OnServerSignal( (int)message.ReadLong() );
            }
        }
    }

    return 1;
}

//-----------------------------------------------------------------------------
// ClientPutInServer hook
//-----------------------------------------------------------------------------

void CServerClientBridge::OnClientPutInServer( edict_t *pPlayer )
{
    g_pServerEngineFuncs->pfnMessageBegin( MSG_ONE, SVC_NEWUSERMSG, NULL, pPlayer );
        g_pServerEngineFuncs->pfnWriteByte( SVC_SVENINT );
        g_pServerEngineFuncs->pfnWriteByte( 255 );
        g_pServerEngineFuncs->pfnWriteLong( 0x6E657653 ); // nevS
        g_pServerEngineFuncs->pfnWriteLong( 0x00746E49 ); // tnI
        g_pServerEngineFuncs->pfnWriteLong( 0x0 );
        g_pServerEngineFuncs->pfnWriteLong( 0x0 );
    g_pServerEngineFuncs->pfnMessageEnd();

    g_SpeedrunTools.SendTimescale( pPlayer );
}

//-----------------------------------------------------------------------------
// Initialize server-client bridge
//-----------------------------------------------------------------------------

void CServerClientBridge::Init( void )
{
	if ( g_pEngineFuncs->HookUserMsg("SvenInt", UserMsgHook_SvenInt) != 0 )
    {
        usermsg_t *pUserMsg = const_cast<usermsg_t *>( Utils()->FindUserMessage("SvenInt") );

        if ( pUserMsg != NULL )
        {
            pUserMsg->function = UserMsgHook_SvenInt;
        }
    }
}

//-----------------------------------------------------------------------------
// Shutdown server-client bridge
//-----------------------------------------------------------------------------

void CServerClientBridge::Shutdown( void )
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
}