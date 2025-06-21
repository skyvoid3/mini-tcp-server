#ifndef NETUTILS_H
#define NETUTILS_H

#include <netdb.h>

// Calling GAI for server server
struct addrinfo *resolve_server_host(const char *port);

// Calling GAI for client host
struct addrinfo *resolve_client_host(const char *host, const char *port);

// Creating socket fd
int create_socket(struct addrinfo *res);

// Struct for accept() returns
typedef struct {
	struct sockaddr_storage addr;
	socklen_t               addr_len;
	int                     accept_fd;
} accept_return_t;

// accept()-ing connections and then using accept_return_t
// to use the info returned by it
accept_return_t accept_connections(int sockfd);

#endif
