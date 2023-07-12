#include <stdio.h>
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

void doGameTick(struct gameState * state)
{
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
		if(state->clients[index].xPosition > 1000)
		{
			state->clients[index].xPosition = 1000;
			state->clients[index].xVelocity = 0;
		}
		if(state->clients[index].xPosition < 0)
		{
			state->clients[index].xPosition = 0;
			state->clients[index].xVelocity = 0;
		}
		if(state->clients[index].yPosition > 1000)
		{
			state->clients[index].yPosition = 1000;
			state->clients[index].yVelocity = 0;
		}
		if(state->clients[index].yPosition < 0)
		{
			state->clients[index].yPosition = 0;
			state->clients[index].yVelocity = 0;
		}
	}
}
