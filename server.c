#include <arpa/inet.h>
#include <errno.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MYPORT "8080"
#define BACKLOG 10

int main() {
	struct addrinfo hints, *res, *p;
	int             sockfd = -1;

	memset(&hints, 0, sizeof hints);
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_UNSPEC;
	hints.ai_protocol = 0;

	int gai_status = getaddrinfo("127.0.0.1", MYPORT, &hints, &res);
	if (gai_status != 0) {
		perror("GAI failed");
		exit(EXIT_FAILURE);
	}

	for (p = res; p != NULL; p = p->ai_next) {
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sockfd == -1) {
			perror("Socket failed");
			continue;
		}

		int yes = 1;
		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == 0) {
			break;
		}
		if (errno == EADDRINUSE) {
			fprintf(stderr, "Port already in use\n");
		}

		close(sockfd);
		sockfd = -1;
	}

	if (sockfd == -1) {
		fprintf(stderr, "No valid socket could be bound\n");
		freeaddrinfo(res);
		exit(EXIT_FAILURE);
	}

	int listen_status = listen(sockfd, BACKLOG);
	if (listen_status == -1) {
		perror("Listen failed");
		close(sockfd);
		freeaddrinfo(res);
		exit(EXIT_FAILURE);
	}
	printf("Listening on port: %s\n", MYPORT);

	struct sockaddr_storage incoming_addr;
	socklen_t               addr_size;
	int                     accept_fd;
	addr_size = sizeof incoming_addr;

	while (1) {
		accept_fd =
		    accept(sockfd, (struct sockaddr *)&incoming_addr, &addr_size);
		if (accept_fd == -1) {
			perror("Accept failed");
			continue;
		}
		printf("Successfuly accepted\n");
		break;
	}

	ssize_t nbytes;
	char    buffer[1024] = "";
	size_t  bufsize = sizeof(buffer);

	while (1) {
		nbytes = recv(accept_fd, buffer, bufsize, 0);
		if (nbytes > 0) {
			break;
		} else if (nbytes == 0) {
			printf("Connection closed\n");
			break;
		} else {
			if (errno == EINTR) {
				// interrupted by signal, retry
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

	fprintf(stdout, "%s", buffer);

	close(accept_fd);
	close(sockfd);
	freeaddrinfo(res);
	return 0;
}
