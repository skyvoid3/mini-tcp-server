#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
	struct addrinfo hints, *res;
	int             sockfd;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int gai_error = getaddrinfo("www.google.com", "https", &hints, &res);
	if (gai_error != 0) {
		fprintf(stderr, "GAI failed: %s\n", gai_strerror(gai_error));
		exit(EXIT_FAILURE);
	}

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd == -1) {
		perror("Socket failed");
		exit(EXIT_FAILURE);
	}

	int yes = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

	int con_err = connect(sockfd, res->ai_addr, res->ai_addrlen);
	if (con_err == -1) {
		perror("Connection failed");
		exit(EXIT_FAILURE);
	}

	printf("Connection successful\n");

	freeaddrinfo(res);
	close(sockfd);

	return 0;
}
