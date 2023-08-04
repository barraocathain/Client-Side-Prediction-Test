#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "cspt-state.h"

void updateInput(struct gameState * state, struct clientInput * message)
{
	if(message->clientNumber < 16 && message->clientNumber >= 0)
	{
		state->clients[message->clientNumber].leftAcceleration = message->left;
		state->clients[message->clientNumber].rightAcceleration = message->right;
		state->clients[message->clientNumber].upAcceleration = message->up;
		state->clients[message->clientNumber].downAcceleration = message->down;		
	}
}

void lerpStates (struct gameState * state, struct gameState * endState)
{
	// Create a copy of the initial state for interpolating to the final state:
	struct gameState * startState = calloc(1, sizeof(struct gameState));
	memcpy(startState, state, sizeof(struct gameState));

	for (double progress = 0.0; progress < 1.0; progress += 0.01)
	{	
		for (uint8_t index = 0; index < 16; index++)
		{
			// Movement:
			state->clients[index].xPosition = startState->clients[index].xPosition +
				(progress * (endState->clients[index].xPosition - startState->clients[index].xPosition));
			state->clients[index].yPosition = startState->clients[index].yPosition +
				(progress * (endState->clients[index].yPosition - startState->clients[index].yPosition));
		}
		usleep(100);
	}
	free(startState);
	memcpy(state, endState, sizeof(struct gameState));
}

void doGameTick(struct gameState * state)
{
	if ((state->tickNumber % UINT64_MAX) == 0)
	{
		state->tickNumber = 0;
	}
	else
	{
		state->tickNumber++;
	}
	for (int index = 0; index < 16; index++)
	{
		// Calculate acceleration:
		if (state->clients[index].leftAcceleration)
		{
			state->clients[index].xVelocity -= 0.5;
		}
		if (state->clients[index].rightAcceleration)
		{
			state->clients[index].xVelocity += 0.5;
		}
		if (!state->clients[index].leftAcceleration && !state->clients[index].rightAcceleration)
		{
			state->clients[index].xVelocity *= 0.9;			
		}
		if (state->clients[index].upAcceleration)
		{
			state->clients[index].yVelocity -= 0.5;
		}
		if (state->clients[index].downAcceleration)
		{
			state->clients[index].yVelocity += 0.5;
		}
		if (!state->clients[index].upAcceleration && !state->clients[index].downAcceleration)
		{
			state->clients[index].yVelocity *= 0.9;			
		}
		
		// Do movement:
		state->clients[index].xPosition += state->clients[index].xVelocity;
		state->clients[index].yPosition += state->clients[index].yVelocity;
		if(state->clients[index].xPosition > 512)
		{
			state->clients[index].xPosition = 0;
		}
		if(state->clients[index].xPosition < 0)
		{
			state->clients[index].xPosition = 512;
		}
		if(state->clients[index].yPosition > 512)
		{
			state->clients[index].yPosition = 0;
		}
		if(state->clients[index].yPosition < 0)
		{
			state->clients[index].yPosition = 512;
		}
	}
}
