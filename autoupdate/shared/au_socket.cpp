#include "../../utils/xorstr.h"

#include "au_socket.h"
#include "au_utils.h"

#include <stdio.h>

CSocketTCP::CSocketTCP()
{
	m_socket = INVALID_SOCKET;
}

socket_t CSocketTCP::Create(const char *pszIpAddress, unsigned short unPort, sockaddr_in_t *name)
{
	name->sin_addr.s_addr = inet_addr( pszIpAddress );
	name->sin_port = htons( unPort );
	name->sin_family = AF_INET;

	m_socket = (socket_t)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	return m_socket;
}

int CSocketTCP::Bind(sockaddr_t *name, int namelen)
{
	return bind(m_socket, name, namelen);
}

int CSocketTCP::Listen()
{
	return listen(m_socket, SOMAXCONN); // SOMAXCONN
}

socket_t CSocketTCP::Accept()
{
	return accept(m_socket, NULL, NULL);
}

int CSocketTCP::Connect(sockaddr_t *name, int namelen)
{
	return connect(m_socket, name, namelen);
}
	
int CSocketTCP::Close(socket_t socket)
{
#ifdef AU_PLATFORM_WINDOWS
	return closesocket(socket);
#else
	return close(socket);
#endif
}

int CSocketTCP::SetOptions(int level, int optname)
{
	int dummy = 1;
	return setsockopt(m_socket, level, optname, (char *)&dummy, sizeof(dummy));
}

int CSocketTCP::Send(socket_t socket, const void *buffer, int len, int flags)
{
	return send(socket, (char *)buffer, len, flags);
}

int CSocketTCP::Recv(socket_t socket, void *buffer, int len, int flags)
{
	return recv(socket, (char *)buffer, len, flags);
}

void CSocketTCP::Initialize()
{
#ifdef AU_PLATFORM_WINDOWS
	WSAData WSAData;
	WSAStartup(0x202, &WSAData);
#endif
}

void CSocketTCP::Shutdown()
{
#ifdef AU_PLATFORM_WINDOWS
	WSACleanup();
#endif
}

int CSocketTCP::GetLastError()
{
#ifdef AU_PLATFORM_WINDOWS
	return WSAGetLastError();
#else
	return errno;
#endif
}

const char *CSocketTCP::GetLastErrorString()
{
#ifdef AU_PLATFORM_WINDOWS
	static char empty_string[] = "";

	TCHAR *pszError;
	int code = WSAGetLastError();

	if ( !FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		code,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPTSTR)&pszError,
		0,
		NULL) )
	{
		return empty_string;
	}

	return pszError;
#else
	return strerror( errno );
#endif
}

void CSocketTCP::FreeErrorString(const char *pszError)
{
#ifdef AU_PLATFORM_WINDOWS
	LocalFree( (HANDLE)pszError );
#endif
}

void CSocketTCP::PrintSocketLastError(const char *pszMessage)
{
	const char *pszError = GetLastErrorString();

	AU_Printf(xs("[%s] ERROR: %s (0x%X)\n"), pszMessage, pszError, GetLastError());

	FreeErrorString( pszError );
}