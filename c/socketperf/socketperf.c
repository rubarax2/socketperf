#undef UNICODE

#define _CRT_SECURE_NO_DEPRECATE
#define WIN32_LEAN_AND_MEAN

#include <time.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <MSWSock.h>
#include <stdlib.h>
#include <stdio.h>

#define MEMORY 0
#define READSEND 1
#define TRANSMITFILE 2

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 1024 *1024
#define DEFAULT_PORT "27015"

void CleanSocketsError(char *msg, SOCKET socket)
{
	printf("%s: %d\n", msg, WSAGetLastError());
	closesocket(socket);
	WSACleanup();
}

void CheckError(int result, char * msg, SOCKET socket)
{
	if (result != SOCKET_ERROR)
		return;

	CleanSocketsError(msg, socket);

	exit(1);
}

void LogTime(char * header, unsigned long length, time_t start)
{
	float totalMB = (float)length / (1024 * 1024);
	time_t time = clock() - start;
	float timeSecs = (float)time / (float)CLOCKS_PER_SEC;
	float speed = totalMB / timeSecs;

	printf("%s %.2f MB in %.2f secs -> %.2f Mbps/s \n", header, totalMB, timeSecs, speed * 8);
}

void InitializeWinsock()
{
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (result == 0)
		return;

	printf("WSAStartup failed with error: %d\n", result);
	exit(1);
}

SOCKET InitializeServerSocketAndListen()
{
	InitializeWinsock();

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	int iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		exit(1);
	}

	// Create a SOCKET for connecting to server
	SOCKET ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		exit(1);
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		freeaddrinfo(result);
		CheckError(iResult, "bind failed with error", ListenSocket);
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);

	CheckError(iResult, "listen failed with error", ListenSocket);

	return ListenSocket;
}

SOCKET InitializeClientSocketAndConnect(char * server)
{
	InitializeWinsock();

	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL, *ptr = NULL, hints;

	int iResult;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(server, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		exit(1);
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			exit(1);
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket != INVALID_SOCKET)
		return ConnectSocket;

	printf("Unable to connect to server!\n");
	WSACleanup();
	exit(1);
}


int Send(SOCKET clientSocket, char * sendbuf, int count)
{
	int result = send(clientSocket, sendbuf, count, 0);

	CheckError(result, "send failed with error", clientSocket);

	return result;
}

void SendDataLength(SOCKET clientSocket, unsigned long length)
{
	unsigned long totalLenNetwork = htonl(length);

	int sent = Send(clientSocket, (char*)(&totalLenNetwork), sizeof(unsigned long));

	if (sent == sizeof(unsigned long))
		return;

	CleanSocketsError("The length header wasn't correctly sent", clientSocket);
	exit(1);
}

unsigned long GetFileLength(FILE * file)
{
	fseek(file, 0L, SEEK_END);
	unsigned long result = ftell(file);
	fseek(file, 0L, SEEK_SET);

	return result;
}

void SendMemory(SOCKET clientSocket, int totalMB)
{
	unsigned long totalLen = totalMB * 1024 * 1024;
	unsigned long totalSent = 0;

	char * sendbuf = (char *)malloc(DEFAULT_BUFLEN);

	time_t start = clock();

	int toSent;
	SendDataLength(clientSocket, totalLen);
	do
	{
		if ((totalLen - totalSent) < DEFAULT_BUFLEN)
			toSent = totalLen - totalSent;
		else
			toSent = DEFAULT_BUFLEN;

		int result = Send(clientSocket, sendbuf, toSent);
		totalSent += result;
	}
	while (totalSent < totalLen);

	LogTime("memory: sent", totalLen, start);
}

void ReadSend(SOCKET clientSocket, char * filePath)
{
	FILE *file;
	char * readBuf = (char *)malloc(DEFAULT_BUFLEN);

	file = fopen(filePath, "rb");
	if (!file)
	{
		CleanSocketsError(filePath, clientSocket);
		printf("Error reading the file: %s\n", filePath);
		exit(1);
	}

	long length = GetFileLength(file);

	time_t start = clock();

	SendDataLength(clientSocket, length);

	int read;
	unsigned long totalRead = 0;
	do
	{
		read = fread(readBuf, sizeof(char), DEFAULT_BUFLEN, file);
		totalRead += read;

		char * sendBuf = readBuf;
		int totalSent = 0;
		do
		{
			int toSend = read - totalSent;
			int sent = Send(clientSocket, sendBuf, toSend);
			totalSent += sent;
			sendBuf += sent;
		}
		while (totalSent < read);
	}
	while (read != 0);

	LogTime("readsend: sent", totalRead, start);

	fclose(file);
}

void SendTransmitFile(SOCKET clientSocket, char * filePath)
{
	FILE *file;

	file = fopen(filePath, "rb");
	if (!file)
	{
		CleanSocketsError(filePath, clientSocket);
		printf("Error reading the file: %s\n", filePath);
		exit(1);
	}

	long length = GetFileLength(file);
	fclose(file);

	time_t start = clock();

	SendDataLength(clientSocket, length);

	HANDLE hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	BOOL success = TransmitFile(clientSocket, hFile, 0, 0, NULL, NULL, TF_USE_KERNEL_APC);

	if (!success)
		CleanSocketsError("Error sending with TransmitFile", clientSocket);

	fclose(file);

	LogTime("transmitfile: sent", length, start);
}

void LaunchServer(int mode, int sizeInMB, char * filePath)
{
	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET clientSocket = INVALID_SOCKET;

	listenSocket = InitializeServerSocketAndListen();

	while (1)
	{
		// Accept a client socket
		clientSocket = accept(listenSocket, NULL, NULL);
		if (clientSocket == INVALID_SOCKET)
		{
			CleanSocketsError("accept failed with error", listenSocket);
			exit(1);
		}

		switch (mode)
		{
		case MEMORY:
			SendMemory(clientSocket, sizeInMB);
			break;

		case READSEND:
			ReadSend(clientSocket, filePath);
			break;

		case TRANSMITFILE:
			SendTransmitFile(clientSocket, filePath);
			break;
		}

		// shutdown the connection since we're done
		int result = shutdown(clientSocket, SD_BOTH);
		CheckError(result, "shutdown failed with error", clientSocket);

		// cleanup
		closesocket(clientSocket);
	}

	// No longer need server socket
	closesocket(listenSocket);
	WSACleanup();
}

void LaunchClient(char * server)
{
	SOCKET ConnectSocket = InitializeClientSocketAndConnect(server);

	char * recvbuf = (char *)malloc(DEFAULT_BUFLEN);

	int iResult;

	time_t start = clock();

	iResult = recv(ConnectSocket, recvbuf, sizeof(unsigned long), 0); //recieve number
	unsigned long totalLen = ntohl(*((unsigned long*)recvbuf));

	unsigned long totalReceived = 0;
	do
	{
		iResult = recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);

		CheckError(iResult, "recv failed with error", ConnectSocket);

		totalReceived += iResult;
	}
	while (totalReceived < totalLen);

	LogTime("received", totalLen, start);

	// shutdown the connection since no more data will be received
	iResult = shutdown(ConnectSocket, SD_BOTH);
	CheckError(iResult, "shutdown failed with error", ConnectSocket);

	// cleanup
	closesocket(ConnectSocket);
	WSACleanup();
}


int __cdecl main(int argc, char **argv)
{
	int mode;
	char * filePath = NULL;
	int sizeInMB = 0;

	char * usageMessage = "usage: \n\
    %s -c server_address\n\
    %s -s [memory size_in_mb | readsend file_path | transmitfile file_path] \n\n\
server modes:\n\
    memory -> the server will send the specified amount of MB directly from memory (max value 4095MB).\n\
    readsend -> the server will read and send the specified file (max file size 2GB).\n\
    transmitfile -> the server will send the specified file using the TransmitFile API call (max file size 2GB).\n";

	// Validate the parameters
	if (argc < 3)
	{
		printf(usageMessage, argv[0], argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "-c") == 0)
	{
		LaunchClient(argv[2]);
		return 0;
	}

	if (strcmp(argv[1], "-s") != 0 || argc != 4)
	{
		printf(usageMessage, argv[0], argv[0]);
		return 1;
	}

	if (strcmp(argv[2], "memory") == 0)
	{
		mode = MEMORY;
		sizeInMB = atoi(argv[3]);
	}
	else if (strcmp(argv[2], "readsend") == 0)
	{
		mode = READSEND;
		filePath = argv[3];
	}
	else if (strcmp(argv[2], "transmitfile") == 0)
	{
		mode = TRANSMITFILE;
		filePath = argv[3];
	}
	else
	{
		printf(usageMessage, argv[0], argv[0]);
		return 1;
	}

	LaunchServer(mode, sizeInMB, filePath);
	return 0;
}