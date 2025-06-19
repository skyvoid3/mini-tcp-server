#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MYPORT "8080"

int main(void) {
	struct addrinfo hints, *res, *p;
	int             sockfd = -1;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;

	int gai_status = getaddrinfo(NULL, MYPORT, &hints, &res);
	if (gai_status != 0) {
		fprintf(stderr, "Gai failed: %s\n", gai_strerror(gai_status));
		exit(EXIT_FAILURE);
	}

	for (p = res; p != NULL; p = p->ai_next) {
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sockfd == -1) {
			perror("Socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
			perror("Bind");
			close(sockfd);
			continue;
		}

		break;
	}

	if (sockfd == -1) {
		fprintf(stderr, "Failed to bind socket\n");
		freeaddrinfo(res);
		exit(EXIT_FAILURE);
	}

	char                   buf[512] = "";
	struct sockaddr_storage incoming_addr;
	socklen_t               addr_len = sizeof incoming_addr;

	printf("UDP server is listening on port: %s\n", MYPORT);

	while (1) {
		ssize_t numbytes =
		    recvfrom(sockfd, buf, sizeof(buf) - 1, 0,
		             (struct sockaddr *)&incoming_addr, &addr_len);

		if (numbytes == -1) {
			perror("Recvfrom");
			continue;
		}

		buf[numbytes] = '\0';



		printf("Received %zd bytes: %s\n", numbytes, buf);

		ssize_t sent = sendto(sockfd, buf, numbytes, 0,
		                      (struct sockaddr *)&incoming_addr, addr_len);


		if (sent == -1) {
			perror("Sendto");
		}
	}

	freeaddrinfo(res);
}
