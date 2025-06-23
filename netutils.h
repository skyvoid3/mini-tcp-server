#ifndef NETUTILS_H
#define NETUTILS_H

#define MAX_MSG 1024
#define MYIP "127.0.0.1"
#include <netdb.h>
#include <signal.h>

/*
 * Sets AI_PASSIVE flag to bind to all available local IP addresses.
 * 
 * Parameters:
 *   port - The service port as a string (e.g., "8080").
 *
 * Returns:
 *   On success: Pointer to a struct addrinfo.
 *   On failure: NULL is returned, and an error message is printed to stderr.
 */
struct addrinfo *resolve_server_host(const char *port);

/*
 * Used for connect() to a TCP server
 *
 * On failure: returns NULL and prints error to stderr 
 *
 * On success: returns pointer to a struct addrinfo
 */
struct addrinfo *resolve_client_host(const char *host, const char *port);

/*
 * Tries to create sockfd using socket(), set SO_REUSEADDR and bind()
 *
 * over every addrinfo returned by gai
 *
 * Returns -1 on failure
 */
int create_server_socket(struct addrinfo *res);

// accept_connections() fills out this struct
typedef struct {
	struct sockaddr_storage addr;
	socklen_t               addr_len;
	int                     accept_fd;
} accept_return_t;

/* Tries to accept() connections
 *
 * Success: returns struct with all clients info (see accept_return_t)
 *
 * Failure: accept_fd == -1 
 */
accept_return_t accept_connections(int sockfd);

/* Choose how many times to try connection and
 The delay between connections (in seconds) */
int retry_connect(struct addrinfo *res, unsigned short tries, unsigned short delay);

/* Gets peer name of ipv4 and ipv6 addresses
 *
 * Prints error message (perror) on failure
*/
void print_peer_name(int sockfd);

/* Pass in the sockfd and port variable to get the port number
 *
 * Returns -1 on err
 *
 */
int get_port(int sockfd, uint16_t *port); 

// Send messages
ssize_t send_message(int sockfd);

// Receive messages
ssize_t recv_message(int sockfd, char *buf);

// Handling SIGHCLD signal and reaping dead child processes
void signal_handler(void);

#endif
