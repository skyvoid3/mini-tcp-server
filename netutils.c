#include "netutils.h"
#include "protocol.h"
#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdbool.h>
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
	hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_flags = AI_PASSIVE; // Use any available ip address on machine

	int gai_status = getaddrinfo(NULL, port, &hints, &res);
	if (gai_status != 0) {
		fprintf(stderr, "GAI: %s\n", gai_strerror(gai_status));

		return NULL;
	}

	return res;
}

struct addrinfo *resolve_client_host(const char *host, uint16_t port) {

	struct addrinfo hints, *res;
	char            port_str[6];

	snprintf(port_str, sizeof port_str, "%u", port);

	memset(&hints, 0, sizeof hints);

	hints.ai_family = AF_UNSPEC;
	hints.ai_protocol = 0;
	hints.ai_socktype = SOCK_STREAM; // TCP

	int gai_status = getaddrinfo(host, port_str, &hints, &res);
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
		int s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

		if (s == -1) {
			perror("Socket");
			continue;
		}

		int yes = 1;
		if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
			perror("Setsockopt");
			close(s);
			continue;
		}
		if (bind(s, p->ai_addr, p->ai_addrlen) == -1) {
			perror("Bind");
			close(s);
			continue;
		}

		sockfd = s;
		break;
	}

	freeaddrinfo(res);

	if (sockfd == -1) {
		return -1;
	}

	if (listen(sockfd, 10) == -1) {
		perror("Listen");
		close(sockfd);
		return -1;
	}

	return sockfd;
}

bool accept_connections(int sockfd, int *conn_count, int *conn_size,
                        connection_info_t **conn_info) {

	int                     new_fd = -1;
	struct sockaddr_storage addr;
	memset(&addr, 0, sizeof addr);
	socklen_t addrlen = sizeof addr;

	while (1) {
		new_fd = accept(sockfd, (struct sockaddr *)&addr, &addrlen);
		if (new_fd == -1) {
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
					return false;
			}
		}
		break;
	}

	// Add new connection information to conn_info array
	add_connection(conn_info, new_fd, conn_count, conn_size, addr, addrlen);

	// Connected to <peer address>
	print_peer_name(new_fd);

	return true;
}

int retry_connect(struct addrinfo *res, unsigned short max_tries,
                  unsigned short delay) {
	char s[INET6_ADDRSTRLEN];

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
				inet_ntop(p->ai_family,
				          get_in_addr((struct sockaddr *)p->ai_addr), s,
				          sizeof s);
				printf("[+] Connected to %s on attempt %hu\n", s, tries + 1);
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

void *get_in_addr(struct sockaddr *sa) {

	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void add_connection(connection_info_t **conn_info, int newfd, int *conn_count,
                    int *conn_size, struct sockaddr_storage addr,
                    socklen_t addrlen) {

	if (*conn_count == *conn_size) { // If no space left 2x the size and realloc
		*conn_size *= 2;
		connection_info_t *temp =
		    realloc(*conn_info, sizeof(connection_info_t) * (*conn_size));
		if (!temp) {
			perror("add_connection() realloc");
			exit(1);
		}
		*conn_info = temp;
	}

	(*conn_info)[*conn_count].pfds.fd = newfd;
	(*conn_info)[*conn_count].pfds.events = POLLIN | POLLOUT;
	(*conn_info)[*conn_count].pfds.revents = 0;
	(*conn_info)[*conn_count].client_info.addr = addr;
	(*conn_info)[*conn_count].client_info.addr_len = addrlen;
	(*conn_info)[*conn_count].client_info.client.state = AWAITING_NAME;
	memset((*conn_info)[*conn_count].client_info.client.name, 0,
	       sizeof((*conn_info)[*conn_count].client_info.client.name));

	(*conn_count)++;
}

void del_connection(connection_info_t conn_info[], int i, int *conn_count) {
	conn_info[i] = conn_info[*conn_count - 1];

	(*conn_count)--;
}

bool handle_client_data(int listener, int *conn_count, int *pfd_i,
                        connection_info_t *conn_info) {

	char buf[MAX_MSG];
	int  sender_fd = conn_info[*pfd_i].pfds.fd;

	ssize_t nbytes_recv = recv(sender_fd, buf, sizeof buf - 1, 0);

	if (nbytes_recv <= 0) {
		if (nbytes_recv == 0) {
			printf("Pollserver: socket %d hung up\n", sender_fd);
		} else {
			perror("handle_client_data recv");
		}

		close(sender_fd);

		del_connection(conn_info, *pfd_i, conn_count);
		return false;

	} else {
		buf[nbytes_recv] = '\0';

		// Parsing the message through protocol
		Command cmd = parse_command(buf, conn_info, pfd_i);

		if (cmd.type == CMD_MSG) {

			int len = strlen(cmd.message);
			printf("%.*s", len, cmd.message); // message string comes with senders name. Sender: <message>
			for (int j = 0; j < *conn_count; j++) {

				int dest_fd = conn_info[j].pfds.fd;

				if (dest_fd != listener && dest_fd != sender_fd) {

					if (sendall(dest_fd, cmd.message, &len) == -1) {
						perror("Send");
					}
				}
			}

		} else if (cmd.type == CMD_QUIT) {

			printf("%s disconnected\n",
			       conn_info[*pfd_i].client_info.client.name);

			del_connection(conn_info, *pfd_i, conn_count);

			int msg_len = strlen(cmd.message);
			for (int j = 0; j < *conn_count; j++) {
				int len = msg_len;

				int dest_fd = conn_info[j].pfds.fd;

				if (dest_fd != listener && dest_fd != sender_fd) {

					if (sendall(dest_fd, cmd.message, &len) == -1) {
						perror("Send");
					}
				}
			}

			close(sender_fd);

			free_command(cmd);

			return false;

		} else {
			char err_buf[63] =
			    "Unknown command\nSend message: MSG <message>\nQuit: QUIT\n";

			int err_buf_len = strlen(err_buf);
			if (sendall(sender_fd, err_buf, &err_buf_len) == -1) {
				perror("Unknown Command Send");
			}
		}
		free_command(cmd);
	}
	return true;
}

void process_connections(int sockfd, int *conn_count, int *conn_size,
                         connection_info_t **conn_info) {

	int i = 0;
	while (i < *conn_count) {
		bool client_still_exists; // If a function fails this sets to false

		if ((*conn_info)[i].pfds.revents & (POLLIN | POLLOUT)) {
			client_still_exists = true;

			if ((*conn_info)[i].client_info.client.state == AWAITING_NAME) { // Check if client has name
				client_still_exists = request_name(*conn_info, i, conn_count);
			}
		}

		if ((*conn_info)[i].pfds.revents & (POLLIN | POLLHUP)) {

			if ((*conn_info)[i].pfds.fd == sockfd) {
				accept_connections(sockfd, conn_count, conn_size, conn_info);

			} else {

				if (!client_still_exists) {
					continue;
				}

				client_still_exists =
				    handle_client_data(sockfd, conn_count, &i, *conn_info);

				if (!client_still_exists) {
					continue;
				}
			}
		}
		i++;
	}
}

int sendall(int s, char *buf, int *len) { // Partial send() handler

	int total = 0;
	int bytesleft = *len;
	int n;

	while (total < *len) {
		n = send(s, buf + total, bytesleft, 0);
		if (n == -1) {
			break;
		}
		total += n;
		bytesleft -= n;
	}

	*len = total;

	return n == -1 ? -1 : 0;
}

bool request_name(connection_info_t *conn_info, int pfd_i, int *conn_count) {

	char prompt[30] = "Please type in your name: ";
	int  len = strlen(prompt);

	if (sendall(conn_info[pfd_i].pfds.fd, prompt, &len) == -1) {
		perror("Request Name Send");
	} 

	char temp[32] = {0};

	ssize_t nbytes = recv(conn_info[pfd_i].pfds.fd, temp, sizeof temp - 1, 0);
	if (nbytes <= 0) {
		if (nbytes == 0) {
			printf("Pollserver: socket %d hung up\n", conn_info[pfd_i].pfds.fd);
		} else {
			perror("Recv");
		}

		del_connection(conn_info, pfd_i, conn_count);
		return false;
	}

	if (nbytes > 1 && temp[nbytes - 1] == '\n') {
		temp[nbytes - 1] = '\0';
		if (temp[nbytes - 2] == '\r') {
			temp[nbytes - 2] = '\0';
		}
	}

	if (strlen(temp) == 0) {
		char empty_name_msg[38] = "Name cannot be empty. Disconnecting.\n";
		int  msg_len = strlen(empty_name_msg);
		sendall(conn_info[pfd_i].pfds.fd, empty_name_msg, &msg_len);

		del_connection(conn_info, pfd_i, conn_count);
		return false;
	}

	snprintf(conn_info[pfd_i].client_info.client.name,
	         sizeof(conn_info[pfd_i].client_info.client.name), "%s", temp);

	char clientIP[INET6_ADDRSTRLEN];

	printf("Client %s set name: %s\n", inet_ntop2(&(conn_info[pfd_i].client_info.addr), clientIP, sizeof clientIP),
	       conn_info[pfd_i].client_info.client.name);

	conn_info[pfd_i].client_info.client.state = READY;

	return true;
}

const char *inet_ntop2(void *addr, char *buf, size_t size)
{
    struct sockaddr_storage *sas = addr;
    struct sockaddr_in *sa4;
    struct sockaddr_in6 *sa6;
    void *src;

    switch (sas->ss_family) {
        case AF_INET:
            sa4 = addr;
            src = &(sa4->sin_addr);
            break;
        case AF_INET6:
            sa6 = addr;
            src = &(sa6->sin6_addr);
            break;
        default:
            return NULL;
    }

    return inet_ntop(sas->ss_family, src, buf, size);
}
