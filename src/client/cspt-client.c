// Client-Side Prediction Test - Client
// Barra Ó Catháin - 2023
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <stdbool.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../cspt-message.h"

int main(int argc, char ** argv)
{
	uint8_t currentPlayerNumber = 0;
	int serverSocket = 0;
	bool continueRunning = true;
	CsptMessage currentMessage;
	struct sockaddr_in serverAddress;
	
	printf("Client-Side Prediction Test - Client Starting.\n");

	// Give me a socket, and make sure it's working:
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == -1)
	{
		printf("Socket creation failed.\n");
		exit(EXIT_FAILURE);
	}
  
	// Set our IP address and port. Default to localhost for testing:
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_port = htons(5200);
  
	// Connect to the server:
	if (connect(serverSocket, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr_in)) != 0)
	{
		fprintf(stderr, "Connecting to the server failed.\n");
		exit(0);
	}

	currentMessage.type = 0;
	currentMessage.content = 0;

	send(serverSocket, &currentMessage, sizeof(CsptMessage), 0);
	recv(serverSocket, &currentMessage, sizeof(CsptMessage), 0);

	if (currentMessage.type == 0)
	{
		currentPlayerNumber = currentMessage.content;
	}

	printf("Registered as: %u\n", currentPlayerNumber);
	printf("%-7s | %u\n", messageStrings[currentMessage.type], currentMessage.content);
	
	while (continueRunning)
	{
		if (recv(serverSocket, &currentMessage, sizeof(CsptMessage), 0) > 0)
		{
			printf("%-7s | %u\n", messageStrings[currentMessage.type], currentMessage.content);
			switch (currentMessage.type)
			{
				case 1:
				{
					// We've been told to disconnect:
					shutdown(serverSocket, SHUT_RDWR);
					serverSocket = 0;
					continueRunning = false;
					break;
				}
				case 2:
				{
					// Pinged, so we now must pong.
					currentMessage.type = 3;
					currentMessage.content = 0;
					send(serverSocket, &currentMessage, sizeof(CsptMessage), 0);
					break;
				}
			}
		}
		else
		{
			shutdown(serverSocket, SHUT_RDWR);
			serverSocket = 0;
			continueRunning = false;
		}
	}
			
	return 0;
}
