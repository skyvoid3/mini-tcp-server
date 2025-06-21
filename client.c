#include "netutils.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MYPORT "8080"

int main(void) {
	struct addrinfo *res = resolve_client_host(MYIP, MYPORT);

	int sockfd = retry_connect(res, 10, 2);

	close(sockfd);
	return 0;
}
