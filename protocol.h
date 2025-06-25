#ifndef PROTOCOL_H
#define PROTOCOL_H

#define CMD_UNKNOWN 0
#define CMD_MSG 1
#define CMD_QUIT 2

typedef struct {
	int type;
	char *message;
} Command;

Command parse_command(const char *input);
void free_command(Command cmd);

#endif
