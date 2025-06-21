#include "netutils.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MYPORT "8080"
#define BACKLOG 10

int main(void) {
	struct addrinfo *res = NULL;
	res = resolve_server_host(MYPORT);
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

	printf("Listening on port: %s\n", MYPORT);

	// Accepting connections
	accept_return_t accept_return = accept_connections(sockfd);

	// Getting the addr info of a client (ipv4 or ipv6)
	struct sockaddr_storage peer_addr;
	socklen_t               peer_addr_len = sizeof(peer_addr);

	if (getpeername(accept_return.accept_fd, (struct sockaddr *)&peer_addr,
	                &peer_addr_len) == 0) {
		if (peer_addr.ss_family == AF_INET) {
			struct sockaddr_in *addr4 = (struct sockaddr_in *)&peer_addr;

			char ipstr[INET_ADDRSTRLEN];

			inet_ntop(AF_INET, &addr4->sin_addr, ipstr, sizeof ipstr);

			printf("Connected to %s:%d\n", ipstr, ntohs(addr4->sin_port));
		} else if (peer_addr.ss_family == AF_INET6) {
			char ipstr6[INET6_ADDRSTRLEN];

			struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&peer_addr;

			inet_ntop(AF_INET6, &addr6->sin6_addr, ipstr6, sizeof ipstr6);

			printf("Connected to %s:%d\n", ipstr6, ntohs(addr6->sin6_port));
		} else {
			perror("getpeername");
		}
	}

	// Receiving messages
	ssize_t nbytes;
	char    buffer[1024] = "";
	size_t  bufsize = sizeof(buffer);

	while (1) {
		nbytes = recv(accept_return.accept_fd, buffer, bufsize, 0);
		if (nbytes > 0) {
			break;
		} else if (nbytes == 0) {
			printf("Connection closed\n");
			break;
		} else {
			if (errno == EINTR) {
				// Interrupted by signal, retry
				continue;
			} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
				// Non-blocking socket: no data available now
				// You can wait or retry later
				continue;
			} else {
				perror("Recv failed");
				break;
			}
		}
	}

	buffer[nbytes] = '\0';
	fprintf(stdout, "Bytes received: %zd Message: %s\n", strlen(buffer), buffer);

	close(accept_return.accept_fd);
	close(sockfd);
	freeaddrinfo(res);
	return 0;
}



