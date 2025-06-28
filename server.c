#include "netutils.h"   // Custom functions
#include <arpa/inet.h>  // For inet_ntop(), inet_pton()
#include <inttypes.h>   // uint16_t etc
#include <netdb.h>      // gai, gethostbyname() also structs
#include <netinet/in.h> // struct sockaddr_in etc
#include <poll.h>       // poll()
#include <signal.h>     // handle signals SIGINT, SIGTERM
#include <stdio.h>      // perror()
#include <stdlib.h>     // Memory managment like malloc() etc
#include <string.h>     // for memset()
#include <sys/socket.h> // Socket API
#include <sys/types.h>  // ssize_t and other types
#include <sys/wait.h>   // for waitpid()
#include <unistd.h>     // POSIX syscalls like close() etc

#define MYPORT "8080"
#define BACKLOG 10

volatile sig_atomic_t stop_server = 0;

void handle_signal(int signo) {
	if (signo == SIGINT || signo == SIGTERM) {
		stop_server = 1;
	}
}

int main(void) {

	struct sigaction sa;
	memset(&sa, 0, sizeof sa);
	sa.sa_handler = handle_signal;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	struct addrinfo *res = resolve_server_host(MYPORT);
	if (res == NULL) {
		exit(EXIT_FAILURE);
	}

	int conn_size = 5;
	int conn_count = 0;
	int sockfd = -1;

	connection_info_t *connections =
	    malloc(sizeof(connection_info_t) * conn_size);
	if (!connections) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	// Creating socket
	sockfd = create_server_socket(res);
	if (sockfd == -1) {
		fprintf(stderr, "Failed to create and bind socket\n");
		exit(EXIT_FAILURE);
	}

	connections[0].pfds.fd = sockfd;
	connections[0].pfds.events = POLLIN;
	connections[0].client_info.client.state = READY;

	conn_count = 1;

	// Getting the port number
	uint16_t port = 0;
	get_port(sockfd, &port);
	printf("Pollserver listening on port: %" PRIu16 "\n", port);

	while (1) {

		struct pollfd *pfds = malloc(sizeof(struct pollfd) * conn_size);
		if (!pfds) {
			perror("Pfds malloc");
			exit(1);
		}

		for (int i = 0; i < conn_count; i++) {
			pfds[i] = connections[i].pfds;
		}

		int poll_count = poll(pfds, conn_count, -1);

		if (stop_server) {
			printf("\n[INFO] Signal received. Shutting down...\n");
			free(pfds);
			break;
		}

		if (poll_count == -1) {
			perror("Poll");
			free(pfds);
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < conn_count; i++) {
			connections[i].pfds.revents = pfds[i].revents;
		}

		free(pfds);

		process_connections(sockfd, &conn_count, &conn_size, &connections);
	}

	cleanup(connections, conn_count);
	close(sockfd);
	return 0;
}
