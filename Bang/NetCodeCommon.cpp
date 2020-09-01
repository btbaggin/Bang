#include "Netcode.h"

static SOCKADDR_IN EndpointToSocket(IPEndpoint* pEndpoint)
{
	SOCKADDR_IN sockaddr_in;
	sockaddr_in.sin_family = AF_INET;
	sockaddr_in.sin_addr.s_addr = htonl(pEndpoint->ip);
	sockaddr_in.sin_port = htons(pEndpoint->port);
	return sockaddr_in;
}
static void EndpointToStr(char* out_str, IPEndpoint pEndpoint)
{
	sprintf(out_str, "%d.%d.%d.%d:%hu",
		 pEndpoint.ip >> 24,
		(pEndpoint.ip >> 16) & 0xFF,
		(pEndpoint.ip >> 8) & 0xFF,
		 pEndpoint.ip & 0xFF,
		 pEndpoint.port);
}

static IPEndpoint Endpoint(u8 a, u8 b, u8 c, u8 d, u16 port)
{
	IPEndpoint ip_endpoint = {};
	ip_endpoint.ip = (a << 24) | (b << 16) | (c << 8) | d;
	ip_endpoint.port = port;
	return ip_endpoint;
}

static bool BindSocket(PlatformSocket* pSocket, IPEndpoint* pEndpoint)
{
	SOCKADDR_IN local_address = EndpointToSocket(pEndpoint);
	if (bind(pSocket->handle, (SOCKADDR*)&local_address, sizeof(local_address)) == SOCKET_ERROR)
	{
		return false;
	}

	return true;
}

static bool SetSocketOption(SOCKET pSocket, int pOpt, int pVal)
{
	int len = sizeof(int);
	if (setsockopt(pSocket, SOL_SOCKET, pOpt, (char*)&pVal, len) == SOCKET_ERROR)
	{
		return false;
	}

	int actual;
	if (getsockopt(pSocket, SOL_SOCKET, pOpt, (char*)&actual, &len) == SOCKET_ERROR)
	{
		return false;
	}

	return pVal == actual;
}

static void NetStart(GameNetState* pState, PlatformSocket* pSocket)
{
	WSADATA winsock_data;
	if (WSAStartup(WINSOCK_VERSION, &winsock_data))
	{
		DisplayErrorMessage("WSAStartup failed: %d", ERROR_TYPE_Error, WSAGetLastError());
		return;
	}

	pSocket->handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (!SetSocketOption(pSocket->handle, SO_RCVBUF, SOCKET_BUFFER_SIZE))
	{
		LogError("Failed to set receive buffer size");
	}
	if (!SetSocketOption(pSocket->handle, SO_SNDBUF, SOCKET_BUFFER_SIZE))
	{
		LogError("Failed to set send buffer size");
	}

	if (pSocket->handle == INVALID_SOCKET)
	{
		DisplayErrorMessage("socket failed: %d", ERROR_TYPE_Error, WSAGetLastError());
		return;
	}

	// put socket in non-blocking mode
	u_long enabled = 1;
	int result = ioctlsocket(pSocket->handle, FIONBIO, &enabled);
	if (result == SOCKET_ERROR)
	{
		DisplayErrorMessage("ioctlsocket failed: %d", ERROR_TYPE_Error, WSAGetLastError());
	}
	pState->initialized = true;
}

static void NetEnd(GameNetState* pState)
{
	pState->initialized = false;
	WSACleanup();
}

static bool SocketReceive(PlatformSocket* pSocket, u8* pBuffer, u32 pBufferSize, u32* pOutSize, IPEndpoint* pFrom)
{
	// get input packet from player
	SOCKADDR_IN from;
	int from_size = sizeof(from);
	int bytes_received = recvfrom(pSocket->handle, (char*)pBuffer, pBufferSize, 0, (SOCKADDR*)&from, &from_size);

	if (bytes_received == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error != WSAEWOULDBLOCK && error != WSAECONNRESET)
		{
			DisplayErrorMessage("recvfrom returned SOCKET_ERROR, WSAGetLastError() %d", ERROR_TYPE_Warning, WSAGetLastError());
		}
		return false;
	}

	if(pOutSize) *pOutSize = bytes_received;

	if (pFrom)
	{
		*pFrom = {};
		pFrom->ip = ntohl(from.sin_addr.S_un.S_addr);
		pFrom->port = ntohs(from.sin_port);
	}

	return true;
}

static bool SocketSend(PlatformSocket* pSocket, IPEndpoint pEndpoint, u8* pBuffer, u32 pBufferSize)
{
	SOCKADDR_IN server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_addr.S_un.S_addr = htonl(pEndpoint.ip);
	server_address.sin_port = htons(pEndpoint.port);
	int server_address_size = sizeof(server_address);

	if (sendto(pSocket->handle, (const char*)pBuffer, pBufferSize, 0, (SOCKADDR*)&server_address, server_address_size) == SOCKET_ERROR)
	{
		return false;
	}
	return true;
}

#define Serialize(pBuffer, data, type) SerializeStruct(pBuffer, data, sizeof(type))
static u32 SerializeStruct(u8** pBuffer, void* s, u32 pSize)
{
	memcpy(*pBuffer, s, pSize);
	*pBuffer += pSize;
	return pSize;
}

#define Deserialize(pBuffer, data, type) DeserializeStruct(pBuffer, data, sizeof(type))
static void DeserializeStruct(u8** pBuffer, void* s, u32 pSize)
{
	memcpy(s, *pBuffer, pSize);
	*pBuffer += pSize;
}

inline static bool IsClientConnected(Client* pClient)
{
#ifdef _SERVER
	return pClient->endpoint.ip != 0;
#else
	return pClient->name[0] != 0;
#endif
}

#include "EventPipe.cpp"
#include "NetMessages.cpp"
