// Client-Side Prediction Test - Server
// Barra Ó Catháin - 2023
#include <time.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <stdbool.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../cspt-message.h"
bool keepRunning = true;

struct connectionStatus
{
	uint8_t remainingPongs;	
};

void sigintHandler(int signal)
{
	keepRunning = false;
}

int main(int argc, char ** argv)
{
	int returnValue = 0;
	int masterSocket = 0;
	int clientSockets[16];
	fd_set connectedClients;
	struct connectionStatus clientStatus[16];
	struct sockaddr_in serverAddress, clientAddress;
	CsptMessage currentMessage;
	printf("Client-Side Prediction Test - Server Starting.\n");

	// Setup the sigint handler:
	signal(SIGINT, sigintHandler);

	// Setup TCP Master Socket:
	printf("Setting up master socket... ");
	masterSocket = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(masterSocket, SOL_SOCKET, SO_REUSEPORT,  &(int){1}, sizeof(int));
	if (masterSocket == -1)
	{
		fprintf(stderr, "Failed to get a socket.\n");
		exit(1);
	}

	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	
	// Set up server address struct:
	bzero(&serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(5200);

	// Bind the socket using the server address struct:
	if (bind(masterSocket, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr_in)) != 0)
	{
		fprintf(stderr, "Failed to bind the socket.\n");
		exit(1);
	}

	// Begin listening:
	if ((listen(masterSocket, 16) != 0))
	{
		fprintf(stderr, "Failed to begin listening.\n");
		exit(1);
	}

	printf("Done!\n");

	for (int index = 0; index < 16; index++)
	{
		clientSockets[index] = 0;
	}

	int clientCount = 0;
	int activityCheck = 0;

	// Prep the file descriptor set:
	FD_ZERO(&connectedClients);
	FD_SET(masterSocket, &connectedClients);

	clientCount = masterSocket;

	time_t lastPingTime;
	time(&lastPingTime);
	
	while (keepRunning)
	{
		FD_ZERO(&connectedClients);
		FD_SET(masterSocket, &connectedClients);
		// Find all sockets that are still working and place them in the set:
		for(int index = 0; index < 16; index++)
		{
			// If it's working, bang it into the list:
			if(clientSockets[index] > 0)
			{
				FD_SET(clientSockets[index], &connectedClients);  
			}
			// The amount of clients is needed for select():
			if(clientSockets[index] > clientCount)
			{
				clientCount = clientSockets[index];
			}
		}
		
		// Check what sockets have items ready to be read:
		timeout.tv_sec = 1;
		activityCheck = select(clientCount + 1, &connectedClients, NULL, NULL, &timeout);

		// Check if select() worked:
		if ((activityCheck < 0) && (errno != EINTR))  
		{ 
			fprintf(stderr, "Error in select(), retrying.\n");  
		}
		if (keepRunning)
		{
			// Check if there are any new connections:
			if (FD_ISSET(masterSocket, &connectedClients))
			{
				// See if we have a slot for the fellow:
				for (int index = 0; index < 16; index++)
				{
					if (clientSockets[index] == 0)
					{
						clientSockets[index] = accept(masterSocket, NULL, NULL);
						FD_SET(clientSockets[index], &connectedClients);
						if(clientSockets[index] > 0)
						{
							printf("Accepted new connection on socket %d.\n", index);
							clientStatus[index].remainingPongs = 0;
						}
						if (clientSockets[index] > masterSocket)
						{
							clientCount = clientSockets[index];
						}
						break;
					}
				}
			}
			
			for (int index = 0; index < 16; index++)
			{
				if (FD_ISSET(clientSockets[index], &connectedClients))
				{
					// The client has sent a message, recieve it and process:
					if ((returnValue = recv(clientSockets[index], &currentMessage, sizeof(CsptMessage), 0)) > 0)
					{
						printf("%s, %u\n", messageStrings[currentMessage.type], currentMessage.content);					
						switch (currentMessage.type)
						{
							// Hello:
							case 0:
							{
								currentMessage.type = 0;
								currentMessage.content = (uint8_t)random() % 16;
								send(clientSockets[index], &currentMessage, sizeof(CsptMessage), 0);
								break;
							}
							// Goodbye:
							case 1:
							{
								FD_CLR(clientSockets[index], &connectedClients);
								shutdown(clientSockets[index], SHUT_RDWR);
								clientSockets[index] = 0;
								break;
							}
							// Ping:
							case 2:
							{
								// Dunno what the client is pingin' me for, so I don't care.
								break;
							}
							// Pong:
							case 3:
							{
								// One less pong to wait on:
								clientStatus[index].remainingPongs--;
								break;
							}
						}
					}				
					else if (returnValue == 0)
					{
						currentMessage.type = 1;
						currentMessage.content = 0;
						FD_CLR(clientSockets[index], &connectedClients);
						send(clientSockets[index], &currentMessage, sizeof(CsptMessage), 0);
						shutdown(clientSockets[index], SHUT_RDWR);
						clientSockets[index] = 0;
					}
				}
			}
			if (time(NULL) >= (lastPingTime + 5))
			{
				time(&lastPingTime);
				for (int index = 0; index < 16; index++)
				{
					if (clientSockets[index] > 0)
					{
						currentMessage.type = 2;
						currentMessage.content = 0;
						send(clientSockets[index], &currentMessage, sizeof(CsptMessage), 0);
						clientStatus[index].remainingPongs++;
						if (clientStatus[index].remainingPongs >= 10)
						{
							currentMessage.type = 1;
							currentMessage.content = 0;
							send(clientSockets[index], &currentMessage, sizeof(CsptMessage), 0);
							shutdown(clientSockets[index], SHUT_RDWR);
							clientSockets[index] = 0;
						}
					}
				}
				printf("Waiting on pongs.\n");
			}
		}
	}
	for (int index = 0; index < 16; index++)
	{
		currentMessage.type = 1;
		currentMessage.content = 0;
		send(clientSockets[index], &currentMessage, sizeof(CsptMessage), 0);
		shutdown(clientSockets[index], SHUT_RDWR);
		clientSockets[index] = 0;
	}
	shutdown(masterSocket, SHUT_RDWR);	
	return 0;
}