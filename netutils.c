#include "netutils.h"
#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

struct addrinfo *resolve_server_host(const char *port) {
	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof hints);

	hints.ai_family = AF_UNSPEC;
	hints.ai_protocol = 0;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int gai_status = getaddrinfo(NULL, port, &hints, &res);
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
	int sockfd = -1;

	for (struct addrinfo *p = res; p != NULL; p = p->ai_next) {
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

		if (sockfd == -1) {
			perror("Socket");
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
			perror("Accept");

			switch (errno) {
				case EINTR:        // Signal interrupt
				case ECONNABORTED: // Connection aborted before accept
					continue;

				case EMFILE: // Process has too many fds
				case ENFILE: // System-wide limit reached
				case ENOMEM: // Kernel ran out of memory
					sleep(1);
					continue;

				default:
					perror("Fatal accept error");
					accept_return.accept_fd = -1;
					return accept_return;
			}
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
void print_peer_name(int sockfd) {
	struct sockaddr_storage peer_addr;
	socklen_t               peer_addr_len = sizeof(peer_addr);

	if (getpeername(sockfd, (struct sockaddr *)&peer_addr, &peer_addr_len) ==
	    0) {
		if (peer_addr.ss_family == AF_INET) {
			struct sockaddr_in *addr4 = (struct sockaddr_in *)&peer_addr;

			char ipstr[INET_ADDRSTRLEN];

			inet_ntop(AF_INET, &addr4->sin_addr, ipstr, INET_ADDRSTRLEN);

			printf("Connected to %s:%d\n", ipstr, ntohs(addr4->sin_port));
		} else if (peer_addr.ss_family == AF_INET6) {
			char ipstr6[INET6_ADDRSTRLEN];

			struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&peer_addr;

			inet_ntop(AF_INET6, &addr6->sin6_addr, ipstr6, INET6_ADDRSTRLEN);

			printf("Connected to %s:%d\n", ipstr6, ntohs(addr6->sin6_port));
		} else {
			perror("Getpeername");
		}
	}
}

int get_port(int sockfd, uint16_t *port) {
	struct sockaddr_storage addr;
	memset(&addr, 0, sizeof addr);
	socklen_t len = sizeof addr;
	if (getsockname(sockfd, (struct sockaddr *)&addr, &len) == -1) {
		return -1;
	}
	if (addr.ss_family == AF_INET) {
		struct sockaddr_in *addr4 = (struct sockaddr_in *)&addr;
		*port = ntohs(addr4->sin_port);
	} else if (addr.ss_family == AF_INET6) {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&addr;
		*port = ntohs(addr6->sin6_port);
	} else {
		return -1;
	}
	return 0;
}

ssize_t send_message(int sockfd) {
	char buffer[1024];

	while (1) {
		fprintf(stdout, "You: ");
		if (fgets(buffer, sizeof buffer, stdin) == 0) {
			return 0;
		} else if (strcmp(buffer, "\n") == 0) {
			return 0;
		}

		size_t len = strlen(buffer);
		size_t total_sent = 0;

		while (total_sent < len) {

			ssize_t nbytes =
			    send(sockfd, buffer + total_sent, len - total_sent, 0);
			if (nbytes == -1) {
				switch (errno) {
					case EINTR:
					case EAGAIN:
						fprintf(stdout, "Trying again\n");
						continue;

					default:
						perror("Send");
						return -1;
				}
			}
			total_sent += nbytes;
		}
		return total_sent;
	}
}

ssize_t recv_message(int sockfd, char *buffer) {
	size_t len = MAX_MSG;

	while (1) {
		ssize_t nbytes = recv(sockfd, buffer, len, 0);
		if (nbytes == -1) {
			switch (errno) {
				case EINTR:
				case EAGAIN:
					fprintf(stdout, "Trying again\n");
					continue;
				default:
					perror("Recv");
					return -1;
			}
		} else if (nbytes == 0) {
			fprintf(stdout, "Connection closed\n");
			return 0;
		}
		buffer[nbytes] = '\0';
		return nbytes;
	}
}

void sigchld_handler(int s) {
	(void)s; // Quiet the unused warning

	// Saving the errno value
	int saved_errno = errno;

	while (waitpid(-1, NULL, WNOHANG) > 0) {
		printf("Child reaped\n");
	}

	errno = saved_errno;
}

void signal_handler(void) {
	struct sigaction sa;

	sa.sa_handler = sigchld_handler;

	sigemptyset(&sa.sa_mask);

	sa.sa_flags = SA_RESTART;

	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}
}
