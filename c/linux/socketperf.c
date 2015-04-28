#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netdb.h>
#include <errno.h>

// change in task

#define MEMORY 0
#define READSEND 1
#define TRANSMITFILE 2

#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define SD_BOTH 2

static int CHUNK_SIZE_IN_KB = 1024;
static char * DEFAULT_PORT = "10001";

#define DEFAULT_BUFLEN CHUNK_SIZE_IN_KB * 1024

void CleanSocketsError(char *msg, int socket)
{
	printf("%s: %s\n", msg, strerror(errno));
	shutdown(socket, SD_BOTH);
	close(socket);
}

void CheckError(int result, char * msg, int socket)
{
	if (result != SOCKET_ERROR)
		return;

	CleanSocketsError(msg, socket);

	exit(1);
}

void LogTime(char * header, unsigned long length, struct timeval start)
{
	float totalMB = (float)length / (1024 * 1024);

	struct timeval end;
	gettimeofday(&end, NULL);

	double timeMillisecs = (end.tv_sec - start.tv_sec) * 1000.0;
	timeMillisecs += (end.tv_usec - start.tv_usec) / 1000.0;

    double timeSecs = timeMillisecs / 1000;
	float speed = totalMB / timeSecs;

	printf("%s %.2f MB in %.2f secs -> %.2f Mbps\n", header, totalMB, timeSecs, speed * 8);
}

// change in main

int InitializeServerSocketAndListen()
{
	struct addrinfo *result = NULL;
	struct addrinfo hints;

	memset(&hints, 0, sizeof hints);

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	int iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		exit(1);
	}

	// Create a SOCKET for connecting to server
	int ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %s\n", strerror(errno));
		freeaddrinfo(result);
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

int InitializeClientSocketAndConnect(char * server)
{
	int ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL, *ptr = NULL, hints;

	int iResult;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(server, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %s\n", strerror(errno));
		exit(1);
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %s\n", strerror(errno));
			exit(1);
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			close(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket != INVALID_SOCKET)
		return ConnectSocket;

	printf("Unable to connect to server!\n");
	exit(1);
}


int Send(int clientSocket, char * sendbuf, int count)
{
	int result = send(clientSocket, sendbuf, count, 0);

	CheckError(result, "send failed with error", clientSocket);

	return result;
}

void SendDataLength(int clientSocket, unsigned long length)
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

void SendMemory(int clientSocket, int totalMB)
{
	unsigned long totalLen = totalMB * 1024 * 1024;
	unsigned long totalSent = 0;

	char * sendbuf = (char *)malloc(DEFAULT_BUFLEN);

	struct timeval start; 
	gettimeofday(&start, NULL);

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

void ReadSend(int clientSocket, char * filePath)
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

	struct timeval start; 
	gettimeofday(&start, NULL);

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

void SendTransmitFile(int clientSocket, char * filePath)
{
	struct stat stat_buf;

	int file = open(filePath, O_RDONLY);
	if (file == -1)
	{
		CleanSocketsError(filePath, clientSocket);
		printf("Error reading the file: %s\n", filePath);
		exit(1);
	}

	fstat(file, &stat_buf);
	unsigned long length = stat_buf.st_size;

	struct timeval start; 
	gettimeofday(&start, NULL);

	SendDataLength(clientSocket, length);

	int sent = sendfile(clientSocket, file, NULL, length);

	if (sent != length)
		CleanSocketsError("Error sending with sendfile", clientSocket);

	close(file);

	LogTime("transmitfile: sent", length, start);
}

void LaunchServer(int mode, int sizeInMB, char * filePath)
{
	int listenSocket = INVALID_SOCKET;
	int clientSocket = INVALID_SOCKET;

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
		close(clientSocket);
	}

	// No longer need server socket
	close(listenSocket);
}

void LaunchClient(char * server)
{
	int ConnectSocket = InitializeClientSocketAndConnect(server);

	char * recvbuf = (char *)malloc(DEFAULT_BUFLEN);

	int iResult;

	struct timeval start; 
	gettimeofday(&start, NULL);

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
	close(ConnectSocket);
}

void SetChunkSizeAndPortIfNeeded(int argc, char **argv, int index)
{
	if (index < argc)
		CHUNK_SIZE_IN_KB = atoi(argv[index]);

	index++;

	if (index < argc)
		DEFAULT_PORT = argv[index];

	printf("Port number %s. Chunk size set to %d KBytes\n", DEFAULT_PORT, CHUNK_SIZE_IN_KB);
}


int main(int argc, char **argv)
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
    transmitfile -> the server will send the specified file using the TransmitFile API call (max file size 2GB).\n\n\
sample: test file transfer using regular sockets:\n\n\
    socketperf.exe -s readsend c:\\data\\server.iso\n\
    readsend : sent 1895.63 MB in 20.11 secs -> 754.14 Mbps\n\n\
    socketperf.exe -c 192.168.1.55\n\
    received 1895.63 MB in 20.11 secs -> 754.14 Mbps\n\n\
sample: test transfer with memory generated data (not reading from disk)\n\n\
    socketperf.exe -s memory 4095\n\
    memory: sent 4095.00 MB in 37.03 secs -> 884.69 Mbps\n\n\
    socketperf.exe -c 192.168.1.55\n\
    received 4095.00 MB in 37.03 secs -> 884.69 Mbps\n\n\
sample: test TransmitFile performance : \n\n\
    socketperf.exe -s transmitfile c:\\data\\server.iso\n\
    transmitfile : sent 1680.31 MB in 21.03 secs -> 639.18 Mbps\n\n\
    socketperf.exe -c 192.168.1.55\n\
    received 1680.31 MB in 21.03 secs -> 639.18 Mbps\n";

	// Validate the parameters
	if (argc < 3)
	{
		printf(usageMessage, argv[0], argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "-c") == 0)
	{
		SetChunkSizeAndPortIfNeeded(argc, argv, 3);
		LaunchClient(argv[2]);
		return 0;
	}

	if (strcmp(argv[1], "-s") != 0 || argc < 4)
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

	SetChunkSizeAndPortIfNeeded(argc, argv, 4);
	LaunchServer(mode, sizeInMB, filePath);
	return 0;
}
/ /  
 