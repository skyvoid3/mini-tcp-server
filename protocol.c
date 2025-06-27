#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Command parse_command(const char *input, connection_info_t *conn_info, int *i) {
	Command     cmd = {CMD_UNKNOWN, NULL};
	const char *src = conn_info[*i].client_info.client.name;

	if (strncmp(input, "MSG", 3) == 0) {
		cmd.type = CMD_MSG;

		// Allocate buffer for "name: message"
		size_t needed = strlen(src) + 2 + strlen(input + 4) +
		                1; // name + ": " + message + '\0'
		cmd.message = malloc(needed);
		if (cmd.message) {
			snprintf(cmd.message, needed, "%s: %s", src, input + 4);
		} else {
			// fallback if malloc fails - avoid null pointer usage later
			cmd.message = strdup("Error allocating memory for message");
		}

	} else if (strncmp(input, "QUIT", 4) == 0) {
		cmd.type = CMD_QUIT;
		char msg[16] = "disconnected\n";
		// Allocate buffer for "disconnected" message
		size_t needed = strlen(src) + strlen(msg) + 2;
		cmd.message = malloc(needed);

		if (cmd.message) {
			snprintf(cmd.message, needed, "%s %s", src, msg);

		} else {
			// fallback if maloc fails
			cmd.message = strdup("Client disconnected");
		}
	}

	return cmd;
}

void free_command(Command cmd) {
	if (cmd.message) {
		free(cmd.message);
	}
}
