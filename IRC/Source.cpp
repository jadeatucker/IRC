

#include <iostream>
#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <cstdlib>


// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

using namespace std;
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "6667"

struct mystruct{
	SOCKET clientsocket;
	char clientname[16];
	char totalrecvbuf[DEFAULT_BUFLEN * 4];
	char tempbuf[DEFAULT_BUFLEN * 4];
	char recvbuf[DEFAULT_BUFLEN];
};

void closeclient(mystruct *client);

vector <mystruct*> clientarray;
mystruct *temp_pointer;
char uservar[] = { 'U', 'S', 'E', 'R', '\0' };
char char_check[5];
SOCKET ClientSocket = INVALID_SOCKET;

int main()
{
	WSADATA wsaData;
	int iResult;
	u_long iMode = 1;
	BOOL bOptVal = TRUE;
	int bOptLen = sizeof(BOOL);

	SOCKET ListenSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;

	int recvbuflen = DEFAULT_BUFLEN;
	int recvbuflenmain = DEFAULT_BUFLEN * 4;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		system("pause");
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		system("pause");
	}
	iResult = ioctlsocket(ListenSocket, FIONBIO, &iMode);
	if (iResult != NO_ERROR)
		printf("ioctlsocket failed with error: %ld\n", iResult);

	iResult = setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&bOptVal, bOptLen);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"setsockopt for SO_REUSEADDR failed with error: %u\n", WSAGetLastError());
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	int totalReceived = 0;
	int currentReceived = 0;

	do{
		//put the socket in a listen state for incoming connections
		iResult = listen(ListenSocket, SOMAXCONN);
		if (iResult == SOCKET_ERROR) {
			printf("listen failed with error: %d\n", WSAGetLastError());
		}

		// Accept a client socket and return a handle for the socket
		ClientSocket = accept(ListenSocket, NULL, NULL);

		if (ClientSocket == INVALID_SOCKET && WSAGetLastError() != WSAEWOULDBLOCK) {
			printf("accept failed with error: %d\n", WSAGetLastError());
		}

		if (ClientSocket != INVALID_SOCKET)
		{
			temp_pointer = new mystruct;
			memset(temp_pointer, 0, sizeof(mystruct));
			temp_pointer->clientsocket = ClientSocket;
			clientarray.push_back(temp_pointer);
			temp_pointer = NULL;
		}

		int array_size = clientarray.size();

		int i = 0;

		// Receive until the peer shuts down the connection
		if (clientarray.size() > 0)
		{
			for (i; i < array_size; i++)
			{
				iSendResult = 0;

				currentReceived = recv(clientarray.at(i)->clientsocket, clientarray.at(i)->recvbuf, recvbuflen, 0);

				if (currentReceived > 0)
				{
					// First this part, a small security check. Making sure that i don't overflow
					// totalrecvbuf. If the amount goes above DEFAULT_BUFLEN *4, make sure i don't actually
					// overfill it.
					if ((totalReceived + currentReceived) > recvbuflenmain)
					{
						currentReceived = recvbuflenmain - totalReceived;
					}

					// This is the transfer from small buffer to big total buffer.
					totalReceived += currentReceived;
					strcat(clientarray.at(i)->tempbuf, clientarray.at(i)->recvbuf);
					if (strchr(clientarray.at(i)->tempbuf, '\r'))
					{
						if (strncmp(uservar, clientarray.at(i)->tempbuf, strlen(uservar)))
						{
							strcat(clientarray.at(i)->totalrecvbuf, "Client#");
							totalReceived += 7;
							char sclientnum[33];
							int clientnum = clientarray.at(i)->clientsocket;
							itoa(clientnum, sclientnum, 10);
							int slength = strlen(sclientnum);
							totalReceived += slength;
							strcat(clientarray.at(i)->totalrecvbuf, sclientnum);
						}
						strncat(clientarray.at(i)->totalrecvbuf, clientarray.at(i)->tempbuf, totalReceived);

						if (!strncmp(uservar, clientarray.at(i)->totalrecvbuf, strlen(uservar)))
						{
							int count = 5;
							for (int j = 0; j < 16; j++, count++){
								clientarray.at(i)->clientname[j] = clientarray.at(i)->totalrecvbuf[count];
								if (clientarray.at(i)->totalrecvbuf[count] == '\r')
								{
									clientarray.at(i)->clientname[j] = '\0';
									break;
								}
							}

							int k = 0;
							char char_check2;
							do{
								char_check2 = clientarray.at(i)->totalrecvbuf[k];
								k++;
							} while (char_check2 != '\r');
							k--;

							memcpy(&clientarray.at(i)->totalrecvbuf[k], " has joined the chat\r\n\0", 23);

							totalReceived += 23;
						}
						for (int j = 0; j < array_size; j++){
							if (i != j)
								iSendResult = send(clientarray.at(j)->clientsocket, clientarray.at(i)->totalrecvbuf, totalReceived, 0);
						}
						printf("Bytes sent: %d\n", iSendResult);
						ZeroMemory(clientarray.at(i)->totalrecvbuf, recvbuflenmain);
						ZeroMemory(clientarray.at(i)->recvbuf, recvbuflen);
						ZeroMemory(clientarray.at(i)->tempbuf, recvbuflenmain);

						currentReceived = 0;
						totalReceived = 0;
					}
				}
				else if (currentReceived == 0)
				{
					closeclient(clientarray.at(i));
					clientarray.erase(clientarray.begin() + i);
					array_size = clientarray.size();

					totalReceived = 0;				
				}
				else if (currentReceived == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
				{
					printf("recv failed with error: %d\n", WSAGetLastError());

					closeclient(clientarray.at(i));
					clientarray.erase(clientarray.begin() + i);
					array_size = clientarray.size();

					totalReceived = 0;
				}
			}
		}
	} while (true);
}

void closeclient(mystruct *client)
{
	int r;
	if (client->clientname != NULL)
	{
		int j;
		for (j = 0; j < sizeof(client->clientname); j++){
			client->totalrecvbuf[j] = client->clientname[j];
			if (client->clientname[j] == '\0')
				break;
		}
		memcpy(&client->totalrecvbuf[j], " has left\r\n\0", 12);
		int namesize = (j + 1) + 12;

		for (int j = 0; j < clientarray.size(); j++){
			if (&client != &clientarray.at(j))
				r = send(clientarray.at(j)->clientsocket, client->totalrecvbuf, namesize, 0);
		}
		printf("Bytes sent: %d\n", r);
	}

	r = shutdown(client->clientsocket, SD_SEND);
	if (r == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
	}
	closesocket(client->clientsocket);
	delete client;
}



