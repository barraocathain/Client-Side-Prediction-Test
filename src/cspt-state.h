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
	uint64_t tickNumber;
	bool left, right, up, down;
};

struct gameState
{
	uint64_t tickNumber;
	struct timeval timestamp; 
	struct clientMovement clients[16];
};

struct networkThreadArguments
{
	int * clientSockets;
	struct gameState * state;
};

struct inputHistory
{
	uint8_t start, end;
	struct clientInput inputs[256];
};
	
void updateInput(struct gameState * state, struct clientInput * message);

void doGameTick(struct gameState * state);

#endif
