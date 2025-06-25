#ifndef NETUTILS_H
#define NETUTILS_H

#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <stdbool.h>

#define MAX_MSG 256
#define MYIP "127.0.0.1"

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
struct addrinfo *resolve_client_host(const char *host, uint16_t port);

/*
 * Tries to create a listening sockfd using socket(), listen(), set SO_REUSEADDR
 * and bind()
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

// Client information
typedef struct {
	int   sockfd;
	char *name;
} client_t;

/* Tries to accept() connections
 *
 * Success: returns struct with all clients info (see accept_return_t)
 *
 * Failure: accept_fd == -1
 */
accept_return_t accept_connections(int sockfd, int *fd_count, int *fd_size,
                                   struct pollfd **pfds);

/* Calls socket() and connect()
 *
 * If no connection was made program exits
 *
 * Returns sockfd on success
 *
 * Parameters:
 *
 * struct addrinfo res
 *
 * uns shrt amount of tries
 *
 * uns shrt delay between tries in seconds
 */
int retry_connect(struct addrinfo *res, unsigned short tries,
                  unsigned short delay);

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

// Returns sin_addr or sin6_addr based for Ipv4 or Ipv6 respectively
void *get_in_addr(struct sockaddr *sa);

/* Adds a new client fd to poll()-ing process
 *
 * If the fd_size == fd_count resizes it by multiplying its capacity
 */
void add_to_pfds(struct pollfd **pfds, int newfd, int *fd_count, int *fd_size);

// Delete client fd from poll()-ing when client disconnects
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count);

/* Receives messages from clients and sends it to everybody else
 *
 * The pfd_i parameter is the index number of socket inside pfds array
 *
 * as determined in the poll() loop inside process_connections function
 */
void handle_client_data(int sockfd, int *fd_count, struct pollfd *pfds,
                        int *pfd_i);

/* Processes every client socket in pfds array.
 *
 * If the socket is the servers listening socket - accepts new connections
 */
void process_connections(int sockfd, int *fd_count, int *fd_size,
                         struct pollfd **pfds);

// send() but cooler
int sendall(int s, char *buf, int *len);

// Request client name
bool request_name(client_t *client, struct pollfd *pfds, int *pfd_i,
                  int *fd_count);

#endif
