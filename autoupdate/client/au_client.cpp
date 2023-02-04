#include <stdio.h>

#include "../../utils/xorstr.h"

#include <steamtypes.h>
#include "../../steam/steam_api.h"
#include "../../friends.h"

#include "au_client.h"

#include "../shared/au_protocol.h"
#include "../shared/au_utils.h"

//-----------------------------------------------------------------------------
// Init, Shutdown
//-----------------------------------------------------------------------------

bool CAUClient::Initialize(const char *pszIpAddress, unsigned short unPort)
{
	CSocketTCP::Initialize();

	int result;

	result = (int)m_SocketTCP.Create(pszIpAddress, unPort, &m_address);
	
	if ( result == INVALID_SOCKET )
	{
		CSocketTCP::PrintSocketLastError(xs("CSocketTCP::Create"));
		return false;
	}
	
	return true;
}

void CAUClient::Shutdown()
{
	m_SocketTCP.Close();

	CSocketTCP::Shutdown();
}

//-----------------------------------------------------------------------------
// Communication bridge with server
// Returns 'true' to keep the client thread alive, otherwise 'false' to terminate the thread
//-----------------------------------------------------------------------------

bool CAUClient::Session(int *code)
{
	// Never was trying to make something similar, so it may look ugly D:

	bool bUpdateAvailable;
	app_version_t version;

	version.major = AUTOUPDATE_APP_MAJOR_VERSION;
	version.minor = AUTOUPDATE_APP_MINOR_VERSION;
	version.patch = AUTOUPDATE_APP_PATCH_VERSION;

	//-----------------------------------------------------------------------------
	// Establish connection with server
	//-----------------------------------------------------------------------------
	
	*code = AUResultCode_NotConnected;

	if ( !EstablishConnection() )
		return false;

	AU_Printf(xs("Connection accepted\n"));

	//-----------------------------------------------------------------------------
	// Send to server our platform
	//-----------------------------------------------------------------------------

	*code = AUResultCode_BadPlatform;

	if ( !SendPlatformType() )
		return false;

	AU_Printf(xs("Transmitted platform type\n"));

	//-----------------------------------------------------------------------------
	// Send app's version to server
	//-----------------------------------------------------------------------------

	*code = AUResultCode_Bad;

	if ( !SendAppVersion(version, &bUpdateAvailable) )
		return false;

	AU_Printf(xs("Client's app version is %s\n"), bUpdateAvailable ? xs("outdated") : xs("up to date"));

	//-----------------------------------------------------------------------------
	// Send to server new query
	//-----------------------------------------------------------------------------

	// Nothing to do here
	if ( !bUpdateAvailable )
	{
		AU_Printf_Clr({ 40, 255, 40, 255 }, xs("[SvenInt::AutoUpdate] Current version is up to date\n"));

		Disconnect(AUResultCode_OK);

		AU_Printf(xs("Disconnected from server\n"));
		return false;
	}

	// Query update
	Protocol_InitPacket(&m_packet, PACKET_QUERY_UPDATE, 0);

	if ( Socket()->Send(&m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CSocketTCP::PrintSocketLastError(xs("CSocketTCP::Send <> PACKET_QUERY_UPDATE"));
		return false;
	}

	*code = AUResultCode_InvalidPacket;

	if ( !RecvPacket() )
		return false;

	if ( m_packet.type == PACKET_DISCONNECT )
	{
		int code;

		if ( Socket()->Recv(&code, sizeof(code), 0) == SOCKET_ERROR )
		{
			CSocketTCP::PrintSocketLastError(xs("CSocketTCP::Recv <> PACKET_DISCONNECT"));
			return false;
		}

		AU_Printf(xs("Server disconnected with code: %d\n"), code);
		return false;
	}

	*code = AUResultCode_Bad;

	// Receiving an update
	RecvUpdate();

	AU_Printf(xs("An update has been received\n"));
	AU_Printf_Clr({ 255, 255, 90, 255 }, xs("[SvenInt::AutoUpdate] New version is available, close game to update\n"));

	//-----------------------------------------------------------------------------
	// Session ends with disconnect packet
	//-----------------------------------------------------------------------------

	int result;

	if ( !RecvDisconnect(&result) )
		return false;

	AU_Printf(xs("Server disconnected with code: %d\n"), result);

	*code = AUResultCode_OK;
	return false;
}

bool CAUClient::RecvUpdate()
{
	extern unsigned char *g_pUpdateData;
	extern unsigned int g_ulUpdateSize;

	unsigned char key[8];

	g_ulUpdateSize = m_packet.length;
	g_pUpdateData = (unsigned char *)malloc(g_ulUpdateSize);

	if ( g_pUpdateData == NULL )
	{
		AU_Printf(xs("Failed to allocate memory\n"));
		return false;
	}

	if ( Socket()->Recv(key, 8, 0) == SOCKET_ERROR )
	{
		CSocketTCP::PrintSocketLastError(xs("CSocketTCP::Recv <> UPDATE"));
		return false;
	}

	int iReceivedBytes = 0;

	while ( iReceivedBytes != g_ulUpdateSize )
	{
		int bytes;

		if ( (bytes = Socket()->Recv(g_pUpdateData + iReceivedBytes, g_ulUpdateSize - iReceivedBytes, 0)) == SOCKET_ERROR )
		{
			AU_Printf_Clr({ 255, 90, 90, 255 }, xs("[SvenInt::AutoUpdate] Update download was interrupted\n"));
			CSocketTCP::PrintSocketLastError(xs("CSocketTCP::Recv <> UPDATE"));
			return false;
		}

		iReceivedBytes += bytes;
	}
	
	DecryptData(g_pUpdateData, key, g_ulUpdateSize, 8);

	return true;
}

bool CAUClient::EstablishConnection()
{
	unsigned long long steamID = XOR_STEAMID( SteamUser()->GetSteamID().ConvertToUint64() );
	Protocol_InitPacket(&m_packet, PACKET_ESTABLISH_CONNECTION, sizeof(steamID));

	if ( Socket()->Send(&m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CSocketTCP::PrintSocketLastError(xs("CSocketTCP::Send <> PACKET_ESTABLISH_CONNECTION"));
		return false;
	}
	
	if ( Socket()->Send(&steamID, sizeof(steamID), 0) == SOCKET_ERROR )
	{
		CSocketTCP::PrintSocketLastError(xs("CSocketTCP::Send <> PACKET_ESTABLISH_CONNECTION"));
		return false;
	}

	return IsServerResponseOK();
}

bool CAUClient::Disconnect(int code)
{
	Protocol_InitPacket(&m_packet, PACKET_DISCONNECT, sizeof(code));

	if ( Socket()->Send(&m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CSocketTCP::PrintSocketLastError(xs("CSocketTCP::Send <> PACKET_DISCONNECT"));
		return false;
	}
	
	if ( Socket()->Send(&code, sizeof(code), 0) == SOCKET_ERROR )
	{
		CSocketTCP::PrintSocketLastError(xs("CSocketTCP::Send <> PACKET_DISCONNECT"));
		return false;
	}

	return true;
}

bool CAUClient::RecvDisconnect(int *code)
{
	*code = AUResultCode_Bad;

	if ( Socket()->Recv(&m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CSocketTCP::PrintSocketLastError(xs("CSocketTCP::Recv <> PACKET_DISCONNECT"));
		return false;
	}

	if ( !Protocol_PacketIsValid(&m_packet) )
	{
		AU_Printf(xs("PACKET_DISCONNECT <> Received invalid packet\n"));
		return false;
	}

	if ( m_packet.type != PACKET_DISCONNECT )
	{
		AU_Printf(xs("PACKET_DISCONNECT <> Invalid packet type\n"));
		return false;
	}

	int response;

	if ( Socket()->Recv(&response, sizeof(response), 0) == SOCKET_ERROR)
	{
		CSocketTCP::PrintSocketLastError(xs("CSocketTCP::Recv <> PACKET_DISCONNECT"));
		return false;
	}

	*code = response;

	return true;
}

bool CAUClient::RecvPacket()
{
	if ( Socket()->Recv(&m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CSocketTCP::PrintSocketLastError(xs("CSocketTCP::Recv <> RecvPacket"));
		return false;
	}

	if ( !Protocol_PacketIsValid(&m_packet) )
	{
		AU_Printf(xs("RecvPacket <> Received invalid packet\n"));
		return false;
	}

	return true;
}

bool CAUClient::SendPlatformType()
{
	int platform;

#ifdef AU_PLATFORM_WINDOWS
	platform = AU_CLIENT_PLATFORM_WINDOWS;
#else
	platform = AU_CLIENT_PLATFORM_LINUX;
#endif

	Protocol_InitPacket(&m_packet, PACKET_PLATFORM, sizeof(platform));

	if ( Socket()->Send(&m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CSocketTCP::PrintSocketLastError(xs("CSocketTCP::Send <> PACKET_PLATFORM"));
		return false;
	}
	
	if ( Socket()->Send(&platform, sizeof(platform), 0) == SOCKET_ERROR )
	{
		CSocketTCP::PrintSocketLastError(xs("CSocketTCP::Send <> PACKET_PLATFORM"));
		return false;
	}

	return IsServerResponseOK();
}

bool CAUClient::SendAppVersion(app_version_t &version, bool *bUpdateAvailable)
{
	*bUpdateAvailable = false;

	Protocol_InitPacket(&m_packet, PACKET_APP_VERSION, sizeof(version));

	if ( Socket()->Send(&m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CSocketTCP::PrintSocketLastError(xs("CSocketTCP::Send <> PACKET_APP_VERSION"));
		return false;
	}

	if ( Socket()->Send(&version, sizeof(version), 0) == SOCKET_ERROR )
	{
		CSocketTCP::PrintSocketLastError(xs("CSocketTCP::Send <> PACKET_APP_VERSION"));
		return false;
	}

	int result, response;
	
	response = RecvServerResponse( &result );

	if ( result == AUResultCode_Bad )
		return false;

	if ( response == AUResultCode_UpdateAvailable )
		*bUpdateAvailable = true;
	else if ( response != AUResultCode_OK )
		return false;

	return true;
}

bool CAUClient::IsServerResponseOK()
{
	int type;
	int length;

	if ( Socket()->Recv(&m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CSocketTCP::PrintSocketLastError(xs("CSocketTCP::Recv <> PACKET_RESPONSE"));
		return false;
	}

	if ( !Protocol_PacketIsValid(&m_packet) )
	{
		AU_Printf(xs("PACKET_RESPONSE <> Received invalid packet\n"));
		return false;
	}

	type = m_packet.type;
	length = m_packet.length;

	if ( type != PACKET_RESPONSE )
	{
		AU_Printf(xs("PACKET_RESPONSE <> Invalid packet type\n"));
		return false;
	}

	int response;

	if ( Socket()->Recv(&response, length, 0) == SOCKET_ERROR )
	{
		CSocketTCP::PrintSocketLastError(xs("CSocketTCP::Recv <> PACKET_RESPONSE"));
		return false;
	}

	if ( response != AUResultCode_OK )
	{
		AU_Printf(xs("PACKET_RESPONSE received error code %d\n"), response);
		return false;
	}

	return true;
}

int CAUClient::RecvServerResponse(int *result)
{
	int type;
	int length;

	*result = AUResultCode_OK;

	if ( Socket()->Recv(&m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CSocketTCP::PrintSocketLastError(xs("CSocketTCP::Recv <> PACKET_RESPONSE"));
		*result = AUResultCode_SocketError;
		
		return AUResultCode_Bad;
	}

	if ( !Protocol_PacketIsValid(&m_packet) )
	{
		AU_Printf(xs("PACKET_RESPONSE <> Received invalid packet\n"));
		*result = AUResultCode_InvalidPacket;
		
		return AUResultCode_Bad;
	}

	type = m_packet.type;
	length = m_packet.length;

	if ( type != PACKET_RESPONSE )
	{
		AU_Printf(xs("PACKET_RESPONSE <> Invalid packet type\n"));
		*result = AUResultCode_InvalidPacketType;
		
		return AUResultCode_Bad;
	}

	int response;

	if ( Socket()->Recv(&response, length, 0) == SOCKET_ERROR )
	{
		CSocketTCP::PrintSocketLastError(xs("CSocketTCP::Recv <> PACKET_RESPONSE"));
		*result = AUResultCode_SocketError;
		
		return AUResultCode_Bad;
	}

	return response;
}