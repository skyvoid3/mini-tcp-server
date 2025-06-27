#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "netutils.h"

#define CMD_UNKNOWN 0
#define CMD_MSG 1
#define CMD_QUIT 2

typedef struct {
	int   type;
	char *message;
} Command;

Command parse_command(const char *input, connection_info_t *conn_info, int *i);
void    free_command(Command cmd);

#endif
