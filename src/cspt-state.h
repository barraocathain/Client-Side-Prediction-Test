#ifndef CSPT_STATE_H
#define CSPT_STATE_H
#include <time.h>
#include <stdbool.h>
#include <sys/time.h>
#include <netinet/in.h>
struct clientMovement
{
	double xPosition, yPosition, xVelocity, yVelocity;
	bool leftAcceleration, rightAcceleration, upAcceleration, downAcceleration, registered;	
};

struct clientInput
{
	int clientNumber;
	bool left, right, up, down;
};

struct gameState
{
	struct timeval timestamp; 
	struct clientMovement clients[16];
};

struct networkThreadArguments
{
	int * clientSockets;
	struct gameState * state;
};

void updateInput(struct gameState * state, struct clientInput * message);

void doGameTick(struct gameState * state);

#endif
