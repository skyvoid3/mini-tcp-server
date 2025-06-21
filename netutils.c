#include "netutils.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

struct addrinfo *resolve_server_host(const char *port) {
	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof hints);

	hints.ai_family = AF_UNSPEC;
	hints.ai_protocol = 0;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int gai_status = getaddrinfo(MYIP, port, &hints, &res);
	if (gai_status != 0) {
		fprintf(stderr, "GAI: %s\n", gai_strerror(gai_status));
		return NULL;
	}

	return res;
}

struct addrinfo *resolve_client_host(const char *host, const char *port) {

	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof hints);

	hints.ai_family = AF_UNSPEC;
	hints.ai_protocol = 0;
	hints.ai_socktype = SOCK_STREAM;

	int gai_status = getaddrinfo(host, port, &hints, &res);
	if (gai_status != 0) {
		fprintf(stderr, "GAI: %s\n", gai_strerror(gai_status));
		return NULL;
	}

	return res;
}

int create_server_socket(struct addrinfo *res) {
	int              sockfd = -1;
	struct addrinfo *p = NULL;

	for (p = res; p != NULL; p = p->ai_next) {
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

		if (sockfd == -1) {
			perror("Socket failed");
			continue;
		}

		int yes = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) ==
		    -1) {
			perror("Setsockopt");
			close(sockfd);
			continue;
		}	
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == 0) {
			printf("Created and bound socket\n");
			return sockfd;
		}

		perror("Bind");
		close(sockfd);

	}

	return -1;
}

accept_return_t accept_connections(int sockfd) {

	accept_return_t accept_return;
	memset(&accept_return, 0, sizeof accept_return);
	socklen_t addr_len = sizeof accept_return.addr;

	while (1) {
		accept_return.accept_fd =
		    accept(sockfd, (struct sockaddr *)&accept_return.addr, &addr_len);
		if (accept_return.accept_fd == -1) {
			perror("Accept failed");
			continue;
		}
		break;
	}
	accept_return.addr_len = addr_len;
	return accept_return;
}

int retry_connect(struct addrinfo *res, unsigned short max_tries,
                  unsigned short delay) {
	for (unsigned short tries = 0; tries < max_tries; tries++) {
		int              sockfd = -1;
		struct addrinfo *p = NULL;

		for (p = res; p != NULL; p = p->ai_next) {
			sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

			if (sockfd == -1) {
				perror("Socket failed");
				continue;
			}

			int yes = 1;
			if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
			               sizeof yes) == -1) {
				perror("Setsockopt");
				close(sockfd);
				continue;
			}
			if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0) {
				printf("[+] Connected on attempt %hu\n", tries + 1);
				return sockfd;
			}
			perror("Connect failed");
			close(sockfd);
		}

		if (tries < max_tries - 1) {
			printf("[-] Retry %hu/%hu\n", tries + 1, max_tries);
			sleep(delay);
		}
	}

	fprintf(stderr, "Out of maximum tries(%hu)\n", max_tries);
	exit(EXIT_FAILURE);
}
