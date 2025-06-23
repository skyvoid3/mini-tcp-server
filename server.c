#include "netutils.h"   // Custom functions
#include <arpa/inet.h>  // For inet_ntop(), inet_pton()
#include <inttypes.h>   // uint16_t etc
#include <netdb.h>      // gai, gethostbyname() also structs
#include <netinet/in.h> // struct sockaddr_in etc
#include <stdio.h>      // perror()
#include <stdlib.h>     // Memory managment like malloc() etc
#include <string.h>
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

	// Handle SIGCHLD, reap zombie childs
	signal_handler();

	// Getting the port number
	uint16_t port = 0;
	get_port(sockfd, &port);
	printf("Listening on port: %" PRIu16 "\n", port);

	// Accepting connections
	accept_return_t accept_return;
	memset(&accept_return, 0, sizeof accept_return);
	while (1) {
		accept_return = accept_connections(sockfd);
		if (accept_return.accept_fd == -1) {
			freeaddrinfo(res);
			close(sockfd);
			exit(EXIT_FAILURE);
		}

		// Printing the addr info of a client (ipv4 or ipv6)
		print_peer_name(accept_return.accept_fd);

		pid_t pid = fork();
		if (pid == -1) {
			perror("Fork");
			close(sockfd);
			close(accept_return.accept_fd);
			freeaddrinfo(res);
			exit(EXIT_FAILURE);
		}

		if (pid == 0) {
			close(sockfd);

			// Receiving and sending messages
			while (1) {
				char    buffer[MAX_MSG];
				ssize_t nbytes_recvd =
				    recv_message(accept_return.accept_fd, buffer);
				if (nbytes_recvd == 0) {
					break;
				} else if (nbytes_recvd == -1) {
					exit(EXIT_FAILURE);
				}
				printf("Guest: %s", buffer);

				while (1) {
					ssize_t nbytes_sent = send_message(accept_return.accept_fd);
					if (nbytes_sent == -1) {
						exit(EXIT_FAILURE);
					} else if (nbytes_sent == 0) {
						fprintf(stdout, "Message cannot be empty\n");
						continue;
					}
					break;
				}
			}
			close(accept_return.accept_fd);
			exit(0);
		} else {
			fprintf(stdout, "Child created pid: %d\n", pid);
			close(accept_return.accept_fd);
			continue;
		}
	}

	close(accept_return.accept_fd);
	close(sockfd);
	freeaddrinfo(res);
	return 0;
}
