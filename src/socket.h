#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <netinet/in.h>
#include <stdint.h>
#include <arpa/inet.h>

/*
 * struct that hold the socket information
 * @fd: socket file descriptor
 * @port: port number in network byte order
 */
struct socket {
	int fd;
	char ip[INET_ADDRSTRLEN];
	uint16_t port;
};

#define SOC_FMT(x) #x": {\n\tfd:\t%d,\n\tport:\t%d\n}\n"
#define SOC_ARGS(x) x.fd, ntohs(x.port)

#define SOC_MAX_QUEUE_SIZE 10

void socket_create(struct socket* soc);
void socket_bind(struct socket* soc, const uint16_t port);
int socket_connect(struct socket* soc, const char* host, const uint16_t port);
int socket_send(struct socket* soc, void* buf, size_t len);
int socket_recv(struct socket* soc, void* buf, size_t* len);
int socket_listen(struct socket* soc);
int socket_accept(struct socket* soc, struct socket* con_soc);

#endif // __SOCKET_H__
