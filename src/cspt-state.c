#include "cspt-state.h"
void updateInput(struct gameState * state, struct clientInput * message, struct sockaddr_in * address, int * clientSockets)
{
	int index = 0;
	struct sockaddr_in currentClientAddress;
	for (index = 0; index < 16; index++)
	{
		getsockname(clientSockets[index], (struct sockaddr *)&currentClientAddress, (socklen_t *)sizeof(struct sockaddr_in));
		if (currentClientAddress.sin_addr.s_addr == address->sin_addr.s_addr)
		{
			state->clients[index].leftAcceleration = message->left;
			state->clients[index].rightAcceleration = message->right;
			state->clients[index].upAcceleration = message->up;
			state->clients[index].downAcceleration = message->down;
			break;
		}
	}
}

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
