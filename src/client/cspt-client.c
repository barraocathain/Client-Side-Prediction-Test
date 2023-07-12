// Client-Side Prediction Test - Client
// Barra Ó Catháin - 2023
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_timer.h>
#include "../cspt-state.h"
#include "../cspt-message.h"

uint8_t colours[16][3] =
{
	{255, 255, 255},
	{100, 176, 254},
	{147, 122, 254},
	{199, 119, 254},
	{243, 106, 254},
	{254, 110, 205},
	{254, 130, 112},
	{235, 159,  35},
	{189, 191,   0},
	{137, 217,   0},
	{93 , 229,  48},
	{69 , 225, 130},
	{72 , 206, 223}
};
// A structure for binding together the shared state between threads:
struct threadParameters
{
	struct gameState * state;
	struct clientInput * message;
	char * ipAddress;
};

void DrawCircle(SDL_Renderer * renderer, int32_t centreX, int32_t centreY, int32_t radius)
{
	const int32_t diameter = (radius * 2);

	int32_t x = (radius - 1);
	int32_t y = 0;
	int32_t tx = 1;
	int32_t ty = 1;
	int32_t error = (tx - diameter);

	while (x >= y)
	{
		//  Each of the following renders an octant of the circle
		SDL_RenderDrawPoint(renderer, centreX + x, centreY - y);
		SDL_RenderDrawPoint(renderer, centreX + x, centreY + y);
		SDL_RenderDrawPoint(renderer, centreX - x, centreY - y);
		SDL_RenderDrawPoint(renderer, centreX - x, centreY + y);
		SDL_RenderDrawPoint(renderer, centreX + y, centreY - x);
		SDL_RenderDrawPoint(renderer, centreX + y, centreY + x);
		SDL_RenderDrawPoint(renderer, centreX - y, centreY - x);
		SDL_RenderDrawPoint(renderer, centreX - y, centreY + x);

		if (error <= 0)
		{
			++y;
			error += ty;
			ty += 2;
		}

		if (error > 0)
		{
			--x;
			tx += 2;
			error += (tx - diameter);
		}
	}
}
	
void * networkHandler(void * parameters)
{
	// Declare the needed variables for the thread:
	struct threadParameters * arguments = parameters;
	struct sockaddr_in serverAddress;
	int udpSocket = 0;

	// Point at the server:
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(arguments->ipAddress);
	serverAddress.sin_port = htons(5200);

	// Create a UDP socket to send through:
	udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

	// Configure a timeout for recieving:
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 1000;
	setsockopt(udpSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	
	struct gameState * updatedState = calloc(1, sizeof(struct gameState));
	while (true)
	{
		// Send our input, recieve the state:
		sendto(udpSocket, arguments->message, sizeof(struct clientInput), 0, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr_in));
		recvfrom(udpSocket, updatedState, sizeof(struct gameState), 0, NULL, NULL);
		if(updatedState->timestamp.tv_sec > arguments->state->timestamp.tv_sec)
		{			
			memcpy(arguments->state, updatedState, sizeof(struct gameState));
		}
		else if(updatedState->timestamp.tv_sec == arguments->state->timestamp.tv_sec &&
			updatedState->timestamp.tv_usec > arguments->state->timestamp.tv_usec)
		{
			memcpy(arguments->state, updatedState, sizeof(struct gameState));
		}		
	}
}

void * gameThreadHandler(void * parameters)
{
	struct threadParameters * arguments = parameters;
	while (true)
	{
		updateInput(arguments->state, arguments->message);
		doGameTick(arguments->state);
		usleep(15625);
	}
}

void * graphicsThreadHandler(void * parameters)
{
	struct gameState * state = ((struct threadParameters *)parameters)->state;
	struct clientInput * message = ((struct threadParameters *)parameters)->message;
	uint32_t rendererFlags = SDL_RENDERER_ACCELERATED;
	
	// Create an SDL window and rendering context in that window:
	SDL_Window * window = SDL_CreateWindow("CSPT-Client", SDL_WINDOWPOS_CENTERED,
										   SDL_WINDOWPOS_CENTERED, 600, 600, 0);
	SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, rendererFlags);
	
	// Enable resizing the window:
	SDL_SetWindowResizable(window, SDL_TRUE);

	SDL_Event event;
	while (true)
	{
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_KEYDOWN:
				{
					switch (event.key.keysym.sym)
					{
						case SDLK_LEFT:
						{
							message->left = true;
							break;
						}
						case SDLK_RIGHT:
						{
							message->right = true;
							break;
						}
						case SDLK_UP:
						{
							message->up = true;
							break;
						}
						case SDLK_DOWN:
						{
							message->down = true;
							break;
						}
						default:
						{
							break;
						}
					}
					break;
				}				
				case SDL_KEYUP:
				{
					switch (event.key.keysym.sym)
					{
						case SDLK_LEFT:
						{
							message->left = false;
							break;
						}
						case SDLK_RIGHT:
						{
							message->right = false;
							break;
						}
						case SDLK_UP:
						{
							message->up = false;
							break;
						}
						case SDLK_DOWN:
						{
							message->down = false;
							break;
						}
					}
					break;
				}
				default:
				{
					break;
				}
			}
		}
		// Clear the screen, filling it with black:		
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		// Draw all the connected clients:
		for (int index = 0; index < 16; index++)
		{
			if (state->clients[index].registered == true)
			{
				SDL_SetRenderDrawColor(renderer, colours[index][0], colours[index][1], colours[index][2], 255);
				DrawCircle(renderer, (long)(state->clients[index].xPosition), (long)(state->clients[index].yPosition), 10);
			}
		}
		
		// Present the rendered graphics:
		SDL_RenderPresent(renderer);

		// Delay enough so that we only hit 144 frames:
		SDL_Delay(1000/144);
	}

	return NULL;
}

int main(int argc, char ** argv)
{
	int serverSocket = 0;
	bool continueRunning = true;
	uint8_t currentPlayerNumber = 0;
	struct sockaddr_in serverAddress;
	struct CsptMessage currentMessage;	
	pthread_t graphicsThread, networkThread, gameThread;
	struct gameState * currentState = calloc(1, sizeof(struct gameState));
	struct clientInput * clientInput = calloc(1, sizeof(struct gameState));   

	// Say hello:
	printf("Client-Side Prediction Test - Client Starting.\n");

	// Give me a socket, and make sure it's working:
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == -1)
	{
		printf("Socket creation failed.\n");
		exit(EXIT_FAILURE);
	}

	// Set our IP address and port. Default to localhost for testing:
	char * ipAddress = calloc(46, sizeof(char));
	if (argc < 1)
	{
		strncpy(ipAddress,"127.0.0.1", 9);
	}
	else
	{
		strncpy(ipAddress, argv[1], strlen(argv[1]));
	}

	// Create an address struct to point at the server:
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(ipAddress);
	serverAddress.sin_port = htons(5200);
  
	// Connect to the server:
	if (connect(serverSocket, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr_in)) != 0)
	{
		fprintf(stderr, "Connecting to the server failed.\n");
		exit(0);
	}

	currentMessage.type = 0;
	currentMessage.content = 0;

	send(serverSocket, &currentMessage, sizeof(struct CsptMessage), 0);
	recv(serverSocket, &currentMessage, sizeof(struct CsptMessage), 0);

	if (currentMessage.type == 0)
	{
		currentPlayerNumber = currentMessage.content;
		clientInput->clientNumber = currentPlayerNumber;
	}

	printf("Registered as: %u\n", currentPlayerNumber);

	// Configure the thread parameters:
	struct threadParameters parameters;
	parameters.message = clientInput;
	parameters.state = currentState;
	parameters.ipAddress = ipAddress;

	// Create all of our threads:
	pthread_create(&gameThread, NULL, gameThreadHandler, &parameters);
	pthread_create(&networkThread, NULL, networkHandler, &parameters);
	pthread_create(&graphicsThread, NULL, graphicsThreadHandler, &parameters);
	
	while (continueRunning)
	{
		if (recv(serverSocket, &currentMessage, sizeof(struct CsptMessage), 0) > 0)
		{
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
					send(serverSocket, &currentMessage, sizeof(struct CsptMessage), 0);
					break;
				}
			}
		}
		else
		{
			// Say goodbye to the server:
			currentMessage.type = 1;
			currentMessage.content = 0;

			// Send the goodbye message and shutdown:
			send(serverSocket, &currentMessage, sizeof(struct CsptMessage), 0);
			shutdown(serverSocket, SHUT_RDWR);
			serverSocket = 0;
			continueRunning = false;
		}
	}
			
	return 0;
}
