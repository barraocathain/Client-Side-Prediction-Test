#ifndef CSPT_MESSAGE_H
#define CSPT_MESSAGE_H
#include <stdint.h>
const char * messageStrings[] = {"HELLO", "GOODBYE", "PING", "PONG"};

struct CsptMessage
{
	uint8_t type;
	uint8_t content;
};

/* Message Types:   
   0 - HELLO: Contents sent to client indicate a given player number.
   1 - GOODBYE: No contents, end the connection.
   2 - PING: Contents indicate the missed amount of pongs.
   3 - PONG: No contents.
*/

#endif
