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

int main() {
	struct addrinfo hints, *res, *p;
	int             sockfd = -1;

	memset(&hints, 0, sizeof hints);
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_UNSPEC;
	hints.ai_protocol = 0;

	int gai_status = getaddrinfo(NULL, MYPORT, &hints, &res);
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
	socklen_t               addr_len;
	int                     accept_fd;

	while (1) {
		addr_len = sizeof incoming_addr;
		accept_fd =
		    accept(sockfd, (struct sockaddr *)&incoming_addr, &addr_len);
		if (accept_fd == -1) {
			perror("Accept failed");
			continue;
		}
		printf("Successfuly accepted\n");
		break;
	}

	struct sockaddr_storage peer_addr;
	socklen_t               peer_addr_len = sizeof(peer_addr);

	if (getpeername(accept_fd, (struct sockaddr *)&peer_addr, &peer_addr_len) ==
	    0) {
		if (peer_addr.ss_family == AF_INET) {
			struct sockaddr_in *addr4 = (struct sockaddr_in *)&peer_addr;
			char                ipstr[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &addr4->sin_addr, ipstr, sizeof ipstr);
			printf("Connected to %s:%d\n", ipstr, ntohs(addr4->sin_port));
		} else if (peer_addr.ss_family == AF_INET6) {
			char                 ipstr6[INET6_ADDRSTRLEN];
			struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&peer_addr;
			inet_ntop(AF_INET6, &addr6->sin6_addr, ipstr6, sizeof ipstr6);
			printf("Connected to %s:%d\n", ipstr6, ntohs(addr6->sin6_port));
		} else {
			perror("getpeername");
		}
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

	buffer[nbytes] = '\0';
	fprintf(stdout, "Bytes received: %zd Message: %s", strlen(buffer), buffer);

	close(accept_fd);
	close(sockfd);
	freeaddrinfo(res);
	return 0;
}
