#ifndef NETUTILS_H
#define NETUTILS_H

#define MYIP "127.0.0.1"
#include <netdb.h>

// Calling GAI for server server
struct addrinfo *resolve_server_host(const char *port);

// Calling GAI for client host
struct addrinfo *resolve_client_host(const char *host, const char *port);

// Creating socket fd
int create_server_socket(struct addrinfo *res);

// Struct for accept() returns
typedef struct {
	struct sockaddr_storage addr;
	socklen_t               addr_len;
	int                     accept_fd;
} accept_return_t;

/* accept()-ing connections and then using accept_return_t
 to use the info returned by it */
accept_return_t accept_connections(int sockfd);

/* Choose how many times to try connection and
 The delay between connections (in seconds) */
int retry_connect(struct addrinfo *res, unsigned short tries, unsigned short delay);


#endif
