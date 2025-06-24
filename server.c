#include "netutils.h"   // Custom functions
#include <arpa/inet.h>  // For inet_ntop(), inet_pton()
#include <inttypes.h>   // uint16_t etc
#include <netdb.h>      // gai, gethostbyname() also structs
#include <netinet/in.h> // struct sockaddr_in etc
#include <poll.h>       // poll()
#include <stdio.h>      // perror()
#include <stdlib.h>     // Memory managment like malloc() etc
#include <sys/socket.h> // Socket API
#include <sys/types.h>  // ssize_t and other types
#include <sys/wait.h>   // for waitpid()
#include <unistd.h>     // POSIX syscalls like close() etc

#define MYPORT "8080"
#define BACKLOG 10

int main(void) {

	struct addrinfo *res = resolve_server_host(MYPORT);
	if (res == NULL) {
		exit(EXIT_FAILURE);
	}

	int            fd_size = 5;
	int            fd_count = 0;
	int            sockfd = -1;
	struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

	// Creating socket
	sockfd = create_server_socket(res);
	if (sockfd == -1) {
		freeaddrinfo(res);
		fprintf(stderr, "Failed to create and bind socket\n");
		exit(EXIT_FAILURE);
	}

	pfds[0].fd = sockfd;
	pfds[0].events = POLLIN;

	fd_count = 1;

	// Getting the port number
	uint16_t port = 0;
	get_port(sockfd, &port);
	printf("Pollserver listening on port: %" PRIu16 "\n", port);

	while (1) {
		int poll_count = poll(pfds, fd_count, -1);

		if (poll_count == -1) {
			perror("Poll");
			exit(EXIT_FAILURE);
		}

		process_connections(sockfd, &fd_count, &fd_size, &pfds);
	}

	free(pfds);
	close(sockfd);
	return 0;
}
