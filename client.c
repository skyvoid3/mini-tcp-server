#include "netutils.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

	if (argc != 3) {
		fprintf(stderr, "Usage: ./client <hostname> <port>\n");
		exit(EXIT_FAILURE);
	}

	int port_int = atoi(argv[2]);
	if (port_int <= 0 || port_int > 65535) {
		fprintf(stderr, "Invalid port number\n");
		exit(EXIT_FAILURE);
	}

	uint16_t port = (uint16_t)port_int;

	struct addrinfo *res = resolve_client_host(argv[1], port);

	int sockfd = retry_connect(res, 10, 2);

	freeaddrinfo(res);

	char buf[1024];

	ssize_t nbytes = recv_message(sockfd, buf);
	if (nbytes == -1) {
		exit(EXIT_FAILURE);
	} else if (nbytes == 0) {
		return 0;
	}

	printf("%s", buf);

	close(sockfd);
	return 0;
}
