#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BACKLOG 10
#define MYPORT "7777"

int main() {

	struct addrinfo hints, *res;
	int             status, sockfd;

	memset(&hints, 0, sizeof hints);

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;

	status = getaddrinfo("127.0.0.1", MYPORT, &hints, &res);
	if (status != 0) {
		fprintf(stderr, "Gai error: %s\n", gai_strerror(status));
		exit(1);
	}

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd == -1) {
		perror("Socket failed");
		freeaddrinfo(res);
		exit(2);
	}

	int yes = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
		perror("Setsockopt failed");
		freeaddrinfo(res);
		close(sockfd);
		exit(3);
	}

	int bind_status = bind(sockfd, res->ai_addr, res->ai_addrlen);
	if (bind_status == -1) {
		perror("Bind failed");
		freeaddrinfo(res);
		close(sockfd);
		exit(4);
	}

	int lstn_err = listen(sockfd, BACKLOG);
	if (lstn_err == -1) {
		perror("Listening failed");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	struct sockaddr_storage incoming_addr;
	socklen_t               addr_size;
	addr_size = sizeof incoming_addr;

	printf("Waiting for connections\n");

	while (1) {

		int accept_fd =
		    accept(sockfd, (struct sockaddr *)&incoming_addr, &addr_size);
		if (accept_fd == -1) {
			perror("Accept failed");
			continue;
		}

		char *msg = "Hello From Narek's TCP client!\n";
		int   len, bytes_sent;

		len = strlen(msg);
		bytes_sent = send(accept_fd, msg, len, 0);
		if (bytes_sent == -1) {
			perror("Send failed");
			close(accept_fd);
			continue;
		}
		printf("Message sent!\n");

		close(accept_fd);
	}

	freeaddrinfo(res);
	close(sockfd);
	return 0;
}
