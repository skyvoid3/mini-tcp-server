#include "netutils.h"  // Custom functions
#include <arpa/inet.h> // For inet_ntop(), inet_pton()
#include <errno.h>     // For errno and error values
#include <inttypes.h>
#include <netdb.h>      // gai, gethostbyname() also structs
#include <netinet/in.h> // struct sockaddr_in etc
#include <stdio.h>      // perror()
#include <stdlib.h>     // Memory managment like malloc() etc
#include <string.h>     // For memset()
#include <sys/socket.h> // Socket API
#include <sys/types.h>  // ssize_t and other types
#include <unistd.h>     // POSIX syscalls like close() etc

#define MYPORT "8080"
#define BACKLOG 10

int main(void) {
	struct addrinfo *res = resolve_server_host(MYPORT);
	if (res == NULL) {
		exit(EXIT_FAILURE);
	}

	int sockfd = -1;

	// Creating socket
	sockfd = create_server_socket(res);
	if (sockfd == -1) {
		freeaddrinfo(res);
		fprintf(stderr, "Failed to create and bind socket\n");
		exit(EXIT_FAILURE);
	}

	// Listening for incoming connections
	int listen_status = listen(sockfd, BACKLOG);
	if (listen_status == -1) {
		perror("Listen failed");
		close(sockfd);
		freeaddrinfo(res);
		exit(EXIT_FAILURE);
	}

	// Getting the port number
	uint16_t port = 0;
	get_port(sockfd, &port);
	printf("Listening on port: %" PRIu16 "\n", port);

	// Accepting connections
	accept_return_t accept_return = accept_connections(sockfd);
	if (accept_return.accept_fd == -1) {
		freeaddrinfo(res);
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	// Printing the addr info of a client (ipv4 or ipv6)
	print_peer_name(accept_return.accept_fd);

	// Receiving and sending messages
	while (1) {
		char buffer[1024];
		ssize_t nbytes = recv_message(accept_return.accept_fd, buffer);
		if (nbytes == 0) {
			return 0;
		}
		printf("Guest: %s", buffer);

		send_message(accept_return.accept_fd);
	}

	close(accept_return.accept_fd);
	close(sockfd);
	freeaddrinfo(res);
	return 0;
}
