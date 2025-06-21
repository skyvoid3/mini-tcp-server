#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

int main() {
	struct addrinfo hints, *res;
	int sockfd;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;


	int gai_status = getaddrinfo(NULL, "7070", &hints, &res);
	if (gai_status != 0) {
		fprintf(stderr, "GAI: %s\n", gai_strerror(gai_status));
		exit(EXIT_FAILURE);
	}

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd == -1) {
		perror("socket");
	}

	int bind_status = bind(sockfd, res->ai_addr, res->ai_addrlen);
	if (bind_status == -1) {
		perror("fuck you");
		close(sockfd);
	}

	int listen_status = listen(sockfd, 100);
	if (listen_status == -1) {
		perror("fuck");
	}

		

	printf("%d\n%d\n", sockfd, bind_status);


	
	return 0;
}
