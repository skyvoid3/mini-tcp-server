#include "protocol.h"
#include <string.h>
#include <stdlib.h>


Command parse_command(const char *input) {
	Command cmd = {CMD_UNKNOWN, NULL};

	if (strncmp(input, "MSG", 3) == 0) {
		cmd.type = CMD_MSG;
		cmd.message = strdup(input + 4);
	} else if (strncmp(input, "QUIT", 4) == 0) {
		cmd.type = CMD_QUIT;
	}

	return cmd;
}

void free_command(Command cmd) {
	if (cmd.type == CMD_MSG && cmd.message) {
		free(cmd.message);

	}
}

