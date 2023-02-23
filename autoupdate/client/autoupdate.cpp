#include "../../utils/xorstr.h"

#include "../shared/au_platform.h"
#include "../shared/au_socket.h"
#include "../shared/au_utils.h"

#include "au_client.h"

#include <stdio.h>

#define AU_USE_SEPARATE_THREAD		( 1 )
#define AU_SERVER_IP				( "45.90.219.164" )
#define AU_SERVER_PORT				( 47101 )

#ifdef AU_PLATFORM_WINDOWS
#include <process.h>
#else
#include <sys/stat.h>
#include <stdlib.h>
#include <pthread.h>
#endif

#ifdef AU_PLATFORM_WINDOWS
#define AU_UPDATER_FILENAME "svenint_autoupdate.exe"
#else
#define AU_UPDATER_FILENAME "svenint_autoupdate"
#endif

CAUClient client_server;
bool g_bAutoUpdateInProcess = false;

// Autoupdate file
unsigned char *g_pUpdateData = NULL;
unsigned int g_ulUpdateSize = 0;

static void Session()
{
	while ( 1 )
	{
		int code;

		bool bIsActive = client_server.Session(&code);

		if ( code != AUResultCode_OK )
		{
			AU_Printf_Clr({ 255, 90, 90, 255 }, xs("[SvenInt::AutoUpdate] Communication with server has broken down (code: %d)\n"), code);
		}

		if ( !bIsActive )
			break;
	}
}

static void AutoUpdate_Client()
{
	if ( client_server.Initialize(xs(AU_SERVER_IP), AU_SERVER_PORT) )
	{
		int connect_result = client_server.Socket()->Connect(client_server.Address(), sizeof(sockaddr_in_t));
		
		if ( connect_result != SOCKET_ERROR )
		{
			AU_Printf(xs("Connected to server\n"));

			Session();
		}
		else
		{
			//AU_Printf("Failed to connect to server\n");
			AU_Printf_Clr({ 255, 90, 90, 255 }, xs("[SvenInt::AutoUpdate] Failed to connect to server\n"));
		}

		client_server.Shutdown();
	}
}

#if AU_USE_SEPARATE_THREAD

#ifdef AU_PLATFORM_WINDOWS
static unsigned int __stdcall AutoUpdate_Thread(void *lpParam)
#else
static void *AutoUpdate_Thread(void *lpParam)
#endif
{
	g_bAutoUpdateInProcess = true;

	AutoUpdate_Client();

	g_bAutoUpdateInProcess = false;

	return 0;
}

#endif

int AutoUpdate()
{
#if !AU_USE_SEPARATE_THREAD

	AutoUpdate_Client();

#else

#ifdef AU_PLATFORM_WINDOWS
	unsigned int ulThreadId;
	_beginthreadex(NULL, 0, &AutoUpdate_Thread, NULL, 0, &ulThreadId);

	return ulThreadId;
#else
	pthread_t clientThread;
	pthread_create(&clientThread, NULL, AutoUpdate_Thread, NULL);

	return (int)clientThread;
#endif

#endif

	return 0;
}

void AutoUpdate_ExtractAndLaunch()
{
	if ( g_pUpdateData != NULL && g_ulUpdateSize > 0 )
	{
		FILE *file = fopen(xs(AU_UPDATER_FILENAME), xs("wb"));

		if ( file != NULL )
		{
			fwrite( g_pUpdateData, sizeof(unsigned char), g_ulUpdateSize, file);
			fclose( file );

		#ifdef AU_PLATFORM_WINDOWS
			//STARTUPINFO startupInfo = { 0 };
			//PROCESS_INFORMATION procInfo = { 0 };

			//if ( CreateProcess(NULL, AU_UPDATER_FILENAME, NULL, NULL, TRUE, 0, NULL, NULL, &startupInfo, &procInfo) )
			//{
			//	WaitForSingleObject( procInfo.hProcess, INFINITE );

			//	CloseHandle( procInfo.hProcess );
			//	CloseHandle( procInfo.hThread );
			//}

			//system( AU_UPDATER_FILENAME );
			system( xs("start " AU_UPDATER_FILENAME) );
		#else
			chmod( xs(AU_UPDATER_FILENAME), S_IRUSR | S_IXUSR );
			system( xs("./" AU_UPDATER_FILENAME) );
		#endif
		}
		else
		{
			//AU_Printf("Failed to extract an update\n");
			AU_Printf_Clr({ 255, 90, 90, 255 }, xs("[SvenInt::AutoUpdate] Failed to extract an update\n"));
		}
	}
}