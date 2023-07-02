#include "cspt-state.h"

void doGameTick(struct gameState * state)
{
	state->timestamp = time(NULL);
	for (int index = 0; index < 16; index++)
	{
		if (state->clients[index].registered == true)
		{
			// Calculate acceleration:
			if (state->clients[index].leftAcceleration)
			{
				state->clients[index].xVelocity -= 0.1;
			}
			if (state->clients[index].rightAcceleration)
			{
				state->clients[index].xVelocity += 0.1;
			}
			if (state->clients[index].upAcceleration)
			{
				state->clients[index].yVelocity += 0.1;
			}
			if (state->clients[index].downAcceleration)
			{
				state->clients[index].yVelocity += 0.1;
			}
			if (!state->clients[index].leftAcceleration && !state->clients[index].rightAcceleration)
			{
				state->clients[index].xVelocity *= 0.1;
			}
			if (!state->clients[index].upAcceleration && !state->clients[index].downAcceleration)
			{
				state->clients[index].yVelocity *= 0.1;
			}

			// Clamp speed:
			if (state->clients[index].xVelocity > 10)
			{
				state->clients[index].xVelocity = 10;
			}
			if (state->clients[index].xVelocity < -10)
			{
				state->clients[index].xVelocity = -10;
			}
			if (state->clients[index].yVelocity > 10)
			{
				state->clients[index].yVelocity = 10;
			}
			if (state->clients[index].yVelocity < -10)
			{
				state->clients[index].yVelocity = -10;
			}
			
			// Do movement:
			state->clients[index].xPosition += state->clients[index].xVelocity;
			state->clients[index].yPosition += state->clients[index].yVelocity;
		}
	}
}
