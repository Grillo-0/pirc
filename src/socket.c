#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/ip.h>

#include <assert.h>

#include "socket.h"
#include "utils.h"

void socket_create(struct socket* soc) {
	int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	EXIT_ON_ERROR(fd == -1);
	soc->fd = fd;
	memset(soc->ip, 0, INET_ADDRSTRLEN);
	soc->port = 0;
}

void socket_bind(struct socket* soc, const uint16_t port) {

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr = { INADDR_ANY }
	};

	int ret = bind(soc->fd, (struct sockaddr*)&addr, sizeof(addr));

	EXIT_ON_ERROR(ret);

	socklen_t addr_len = sizeof(addr);
	EXIT_ON_ERROR(getsockname(soc->fd, (struct sockaddr*)&addr, &addr_len));
	EXIT_ON_ERROR(!inet_ntop(AF_INET, &addr.sin_addr, soc->ip, INET_ADDRSTRLEN));
	soc->port = ntohs(addr.sin_port);
}

int socket_connect(struct socket* soc, const char* host, const uint16_t port) {
	struct addrinfo hints = { 0 };
	struct addrinfo* result;

	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;          /* Any protocol */
	hints.ai_flags = 0;    /* For wildcard IP address */
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	char port_string[32];
	snprintf(port_string, 32, "%d", port);

	EXIT_ON_ERROR(getaddrinfo(host, port_string, &hints, &result));

	int connect_res = -1;
	for(struct addrinfo* it = result; it != NULL; it = it->ai_next) {
		if ((connect_res = connect(soc->fd, it->ai_addr, it->ai_addrlen)) != -1)
			break;
	}

	return connect_res;
}

int socket_send(struct socket* soc, void* buf, size_t len) {
	ssize_t sent_len = 0;
	while((size_t)sent_len != len) {
		sent_len = send(soc->fd, buf, len, 0);
		if (sent_len == -1)
			return -1;
	}
	return 0;
}

int socket_recv(struct socket* soc, void* buf, size_t* len) {
	ssize_t ret = recv(soc->fd, buf, *len, 0);
	if (ret < 0)
		return ret;

	*len = ret;
	return 0;
}

int socket_listen(struct socket* soc) {
	return listen(soc->fd, SOC_MAX_QUEUE_SIZE);
}

int socket_accept(struct socket* soc, struct socket* con_soc) {
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);

	int fd = accept(soc->fd, (struct sockaddr*)&addr, &addr_len);

	assert(addr_len == sizeof(addr));
	if (fd == -1)
		return -1;

	con_soc->fd = fd;
	EXIT_ON_ERROR(!inet_ntop(AF_INET, &addr.sin_addr, con_soc->ip, INET_ADDRSTRLEN));
	con_soc->port = ntohs(addr.sin_port);

	return 0;
}
