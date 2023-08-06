/* /======================================\
   | Client-Side Prediction Test - Client |
   | Barra Ó Catháin - 2023               |
   \======================================/ */
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

#define ENABLE_CLIENT_SIDE_PREDICTION
#define ENABLE_SERVER_RECONCILLIATION

// A structure for binding together the shared state between threads:
struct ClientThreadParameters
{
	char * ipAddress;
	bool * keepRunning;
	struct gameState * state;
	struct clientInput * message;
	struct inputHistory * inputBuffer;
};

// Seperate colours to distinguish each of the 16 possible clients:
static const uint8_t colours[16][3] =
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

// Draws a circle based on the midpoint circle algorithm:
void drawCircle(SDL_Renderer * renderer, int32_t centreX, int32_t centreY, int32_t radius)
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
	// Unpack the variables passed to the thread:
	char * ipAddress = ((struct ClientThreadParameters * )parameters)->ipAddress;
	bool * keepRunning = ((struct ClientThreadParameters * )parameters)->keepRunning;
	struct gameState * state = ((struct ClientThreadParameters * )parameters)->state;
	struct clientInput * message = ((struct ClientThreadParameters * )parameters)->message;
	struct inputHistory * inputBuffer = ((struct ClientThreadParameters * )parameters)->inputBuffer;

	// Point at the server:
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(ipAddress);
	serverAddress.sin_port = htons(5200);

	// Create a UDP socket to send through:
	int udpSocket = 0;
	udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

	// Configure a timeout for receiving:
	struct timeval receiveTimeout;
	receiveTimeout.tv_sec = 0;
	receiveTimeout.tv_usec = 1000;
	setsockopt(udpSocket, SOL_SOCKET, SO_RCVTIMEO, &receiveTimeout, sizeof(struct timeval));

	// A structure to store the most recent state from the network:
	struct gameState * updatedState = calloc(1, sizeof(struct gameState));
	
	while (keepRunning)
	{
		// Send our input, recieve the state:
		sendto(udpSocket, message, sizeof(struct clientInput), 0, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr_in)); 
		recvfrom(udpSocket, updatedState, sizeof(struct gameState), 0, NULL, NULL);

		// Only update the state if the given state is more recent than the current state:	   
		if (updatedState->timestamp.tv_sec > state->timestamp.tv_sec ||
			(updatedState->timestamp.tv_sec == state->timestamp.tv_sec &&
			 updatedState->timestamp.tv_usec > state->timestamp.tv_usec))
		{		
			#ifdef ENABLE_SERVER_RECONCILLIATION
			// Throw away any already acknowledged inputs:
			while (inputBuffer->start != -1 && inputBuffer->inputs[inputBuffer->start].tickNumber < state->tickNumber)
			{				
				inputBuffer->start = (inputBuffer->start + 1) % 256;
				if(inputBuffer->start == inputBuffer->end)
				{
					inputBuffer->start = -1;
				}
			}

			uint8_t currentMessage = inputBuffer->start;
			uint64_t lastTickNumber = inputBuffer->inputs[inputBuffer->start].tickNumber;

			// Re-apply the currently unused messages:
			while (currentMessage != 1 && currentMessage != inputBuffer->end)
			{
				updateInput(state, &inputBuffer->inputs[currentMessage]);
				currentMessage = (currentMessage + 1) % 256;

				// When we get to the next tick in the inputs, apply a game tick:
				if (inputBuffer->inputs[currentMessage].tickNumber != lastTickNumber)
				{
					doGameTick(state);
				}
			}	
			#endif
			
			// Interpolate to the new state:
			lerpStates(state, updatedState);
		}
	}
	return NULL;
}

void * gameThreadHandler(void * parameters)
{
	// Unpack the variables passed to the thread:
	bool * keepRunning = ((struct ClientThreadParameters * )parameters)->keepRunning;
	struct gameState * state = ((struct ClientThreadParameters * )parameters)->state;
	struct clientInput * message = ((struct ClientThreadParameters * )parameters)->message;
	struct inputHistory * inputBuffer = ((struct ClientThreadParameters * )parameters)->inputBuffer;

	#ifdef ENABLE_CLIENT_SIDE_PREDICTION
	struct gameState * nextStep = calloc(1, sizeof(struct gameState));
	while (keepRunning)
	{
		updateInput(state, message);
		
		#ifdef ENABLE_SERVER_RECONCILLIATION
		if(inputBuffer->start == -1)
		{
			memcpy(&inputBuffer->inputs[0], message, sizeof(struct clientInput));
			inputBuffer->start = 0;
			inputBuffer->end = 1;
		}
		else
		{
			memcpy(&inputBuffer->inputs[inputBuffer->end], message, sizeof(struct clientInput));
			inputBuffer->end = (inputBuffer->end + 1) % 256;
		}		
		#endif
		
		memcpy(nextStep, state, sizeof(struct gameState));
		doGameTick(nextStep);
		lerpStates(state, nextStep);
		usleep(15625);
	}
	#endif

	return NULL;
}

void * graphicsThreadHandler(void * parameters)
{
	bool * keepRunning = ((struct ClientThreadParameters *)parameters)->keepRunning;
	struct gameState * state = ((struct ClientThreadParameters *)parameters)->state;
	struct clientInput * message = ((struct ClientThreadParameters *)parameters)->message;
	uint32_t rendererFlags = SDL_RENDERER_ACCELERATED;
	
	// Create an SDL window and rendering context in that window:
	SDL_Window * window = SDL_CreateWindow("CSPT-Client", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 512, 512, 0);
	SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, rendererFlags);	
	SDL_Event event;
	
	while (keepRunning)
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
							message->tickNumber = state->tickNumber;
							message->left = true;
							break;
						}
						case SDLK_RIGHT:
						{
							message->tickNumber = state->tickNumber;
							message->right = true;
							break;
						}
						case SDLK_UP:
						{
							message->tickNumber = state->tickNumber;
							message->up = true;
							break;
						}
						case SDLK_DOWN:
						{
							message->tickNumber = state->tickNumber;
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
							message->tickNumber = state->tickNumber;
							message->left = false;
							break;
						}
						case SDLK_RIGHT:
						{
							message->tickNumber = state->tickNumber;
							message->right = false;
							break;
						}
						case SDLK_UP:
						{
							message->tickNumber = state->tickNumber;
							message->up = false;
							break;
						}
						case SDLK_DOWN:
						{
							message->tickNumber = state->tickNumber;
							message->down = false;
							break;
						}
					}
					break;
				}
				case SDL_QUIT:
				{
					*keepRunning = false;
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
				// Set the colour to the correct one for the client:
				SDL_SetRenderDrawColor(renderer, colours[index][0], colours[index][1], colours[index][2], 255);

				// Draw the circle:
				drawCircle(renderer, (long)(state->clients[index].xPosition), (long)(state->clients[index].yPosition), 10);

				// Draw an additional circle so we can tell ourselves apart from the rest:
				if (index == message->clientNumber)
				{
					drawCircle(renderer, (long)(state->clients[index].xPosition), (long)(state->clients[index].yPosition), 5);	  
				}
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
	bool keepRunning = true;
	uint8_t currentPlayerNumber = 0;
	struct sockaddr_in serverAddress;
	struct CsptMessage currentMessage;	   
	struct gameState * currentState = calloc(1, sizeof(struct gameState));
	struct clientInput * clientInput = calloc(1, sizeof(struct gameState));   

	// Print a welcome message:
	printf("Client-Side Prediction Test - Client Starting.\n");
	printf("==============================================\n");

	// Print a list of enabled features:
	#ifdef ENABLE_CLIENT_SIDE_PREDICTION
	printf("Client-side prediction is enabled in this build.\n");
	#endif
	#ifdef ENABLE_SERVER_RECONCILLIATION
	printf("Server reconcilliation is enabled in this build.\n");
	#endif
	
	// Give me a socket, and make sure it's working:
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == -1)
	{
		printf("Socket creation failed.\n");
		exit(EXIT_FAILURE);
	}

	// Set our IP address and port. Default to localhost for testing:
	char * ipAddress = calloc(46, sizeof(char));
	if (argc < 2)
	{
		strncpy(ipAddress, "127.0.0.1", 10);
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

	printf("Joined server as client: %u.\n", currentPlayerNumber);

	// Configure the thread parameters:
	struct ClientThreadParameters parameters;
	parameters.state = currentState;
	parameters.message = clientInput;
	parameters.ipAddress = ipAddress;
	parameters.keepRunning = &keepRunning;
	parameters.inputBuffer = calloc(1, sizeof(struct inputHistory));
	parameters.inputBuffer->start = -1;
	parameters.inputBuffer->end = -1;
	
	// Create all of our threads:
	pthread_t graphicsThread, networkThread, gameThread;
	pthread_create(&gameThread, NULL, gameThreadHandler, &parameters);
	pthread_create(&networkThread, NULL, networkHandler, &parameters);
	pthread_create(&graphicsThread, NULL, graphicsThreadHandler, &parameters);
	
	while (keepRunning)
	{
		if (recv(serverSocket, &currentMessage, sizeof(struct CsptMessage), 0) > 0)
		{
			switch (currentMessage.type)
			{
				// Recieved a "GOODBYE" message:
				case 1:
				{
					// Close the socket, and stop the client:
					shutdown(serverSocket, SHUT_RDWR);
					serverSocket = 0;
					keepRunning = false;
					
					break;
				}
				// Recieved a "PING" message:
				case 2:
				{
					// Setup and send a "PONG" message:
					currentMessage.type = 3;
					currentMessage.content = 0;
					send(serverSocket, &currentMessage, sizeof(struct CsptMessage), 0);
					
					break;
				}
			}
		}

		// If we've lost connection for some reason:
		else
		{
			// Setup a "GOODBYE" message:
			currentMessage.type = 1;
			currentMessage.content = 0;

			// Send the "GOODBYE" message and shutdown the socket:
			send(serverSocket, &currentMessage, sizeof(struct CsptMessage), 0);
			shutdown(serverSocket, SHUT_RDWR);
			serverSocket = 0;
			keepRunning = false;
		}
	}

	// Say goodbye to the server:
	currentMessage.type = 1;
	currentMessage.content = 0;

	// Send the goodbye message and shutdown:
	send(serverSocket, &currentMessage, sizeof(struct CsptMessage), 0);
	shutdown(serverSocket, SHUT_RDWR);
	serverSocket = 0;
	keepRunning = false;
			
	return 0;
}
