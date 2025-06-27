#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "netutils.h"

#define CMD_UNKNOWN 0
#define CMD_MSG 1
#define CMD_QUIT 2

// Protocol struct
typedef struct {
	int   type;
	char *message;
} Command;

// Parsing incoming string through protocol
Command parse_command(const char *input, connection_info_t *conn_info, int *i);

// Free the command
void free_command(Command cmd);

#endif
