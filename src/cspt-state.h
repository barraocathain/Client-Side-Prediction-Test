#ifndef CSPT_STATE_H
#define CSPT_STATE_H
#include <time.h>
#include <stdbool.h>
#include <netinet/in.h>
struct clientMovement
{
	double xPosition, yPosition, xVelocity, yVelocity;
	bool leftAcceleration, rightAcceleration, upAcceleration, downAcceleration, registered;	
};

struct clientInput
{
	bool left, right, up, down;
};

struct gameState
{
	time_t timestamp; 
	struct clientMovement clients[16];
};

struct networkThreadArguments
{
	int * clientSockets;
	struct gameState * state;
};

void updateInput(struct gameState * state, struct clientInput * message, struct sockaddr_in * address, int * clientSockets);

void doGameTick(struct gameState * state);

#endif
