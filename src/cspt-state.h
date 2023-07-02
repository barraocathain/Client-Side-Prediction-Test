#ifndef CSPT_STATE_H
#define CSPT_STATE_H
#include <time.h>
#include <stdbool.h>

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

void doGameTick(struct gameState * state);

#endif
