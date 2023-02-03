#ifndef AU_SERVER_H
#define AU_SERVER_H

#ifdef _WIN32
#pragma once
#endif

#include <mutex>

#include "../shared/au_socket.h"
#include "../shared/au_protocol.h"

#include "au_client.h"

#include "../shared/au_app_info.h"
#include "../shared/au_app_version.h"

class CAUServer
{
public:
	bool Initialize(const char *pszIpAddress, unsigned short unPort);
	void Shutdown();

	bool ClientSession(CAUClient &client);
	bool EstablishConnection(CAUClient &client);
	bool Disconnect(CAUClient &client, int code);
	bool RecvPlatformType(CAUClient &client);
	bool RecvAppVersion(CAUClient &client, app_version_t &client_app_version);
	bool SendResponse(CAUClient &client, int code);
	bool SendUpdate(CAUClient &client);

	inline CSocketTCP *Socket() { return &m_SocketTCP; };
	inline sockaddr_in_t *Address() { return &m_address; };

private:
	bool IsVersionOutdated(app_version_t &server, app_version_t &client);

private:
	CSocketTCP m_SocketTCP;
	sockaddr_in_t m_address;
	AUPacket m_packet;

	std::mutex m_UpdateMutex;
};

#endif