#include <asm-generic/socket.h>
#include <pthread.h>

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "../socket.h"
#include "../utils.h"
#include "../msg.h"

struct msg_queue {
	struct msg msg;
	struct msg_queue* next;
};

struct client {
	pthread_t id;
	struct socket soc;
	char* nickname;
};

struct client client_create(struct socket* soc) {
	struct client result  = {
		.id = 0,
		.soc = *soc
	};

	return result;
}

struct client_list {
	struct client cli;
	struct client_list* next;
	struct client_list* back;
};

pthread_mutex_t in_queue_mutex;
pthread_cond_t in_queue_cond;
struct msg_queue* in_queue = NULL;

pthread_mutex_t clients_mutex;
struct client_list* clients = NULL;

void handle_msg(struct client* cli, struct msg* msg) {
	struct msg res;

	// If a client is trying to send some message before connecting just
	// ignore it
	if (cli->nickname == NULL && msg->cmd != CMD_CONNECT)
		return;

	switch (msg->cmd) {
	case CMD_MSG:
		pthread_mutex_lock(&in_queue_mutex);
		struct msg_queue* node = malloc(sizeof(struct msg_queue));
		node->msg = *msg;
		node->next = in_queue;
		in_queue = node;
		pthread_cond_signal(&in_queue_cond);
		pthread_mutex_unlock(&in_queue_mutex);
		return;
	case CMD_PING:
		msg_create(&res, NULL, CMD_PONG);
		msg_send(&cli->soc, &res);
		msg_free(&res);
		break;
	case CMD_CONNECT:
		cli->nickname = sv_to_cstr(msg->prefix);
		break;
	case CMD_PONG:
	case CMD_INVALID:
	case CMD_SIZE:
	default:
		break;
	}
	msg_free(msg);
}

void* handle_client(void* arg) {
	struct client cli = *(struct client*)arg;

	struct msg received;
	while (1) {
		if (msg_recv(&cli.soc, &received)) {
			pthread_mutex_lock(&clients_mutex);
			struct client_list* node;
			for (node = clients; node; node = node->next) {
				if (node->cli.id == cli.id)
					break;
			}

			if (!node) {
				pthread_mutex_unlock(&clients_mutex);
				break;
			}

			if (node->back)
				node->back->next = node->next;
			else
				clients = node->next;

			pthread_mutex_unlock(&clients_mutex);
			break;
		}

		handle_msg(&cli, &received);
	}
	return NULL;
}

void* handle_incoming_messages(void* arg) {
	(void)arg;

	while (1) {
		pthread_mutex_lock(&in_queue_mutex);
		while (!in_queue) {
			pthread_cond_wait(&in_queue_cond, &in_queue_mutex);
		}

		for (struct client_list* node = clients; node; node = node->next)
			msg_send(&node->cli.soc, &in_queue->msg);

		msg_free(&in_queue->msg);

		in_queue = in_queue->next;
		pthread_mutex_unlock(&in_queue_mutex);
	}

	return NULL;
}

struct socket serv_soc;
void handle_exit(int signum) {
	(void)signum;

	close(serv_soc.fd);
	for (struct client_list* node = clients; node; node = node->next)
		close(node->cli.soc.fd);

	exit(EXIT_SUCCESS);
}

int main(void) {

	socket_create(&serv_soc);
	int reuse = 1;
	EXIT_ON_ERROR(setsockopt(serv_soc.fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)));
	socket_bind(&serv_soc, 42069);

	signal(SIGINT, handle_exit);

	EXIT_ON_ERROR(socket_listen(&serv_soc));

	pthread_mutex_init(&in_queue_mutex, NULL);

	pthread_mutex_init(&clients_mutex, NULL);

	pthread_t id;
	EXIT_ON_ERROR(pthread_create(&id, NULL, handle_incoming_messages, NULL));
	pthread_detach(id);

	while (1) {
		struct socket client_soc;
		pthread_t client_id;
		EXIT_ON_ERROR(socket_accept(&serv_soc, &client_soc));

		struct client cli = client_create(&client_soc);
		EXIT_ON_ERROR(pthread_create(&client_id, NULL, handle_client, &cli));
		pthread_detach(client_id);
		cli.id = client_id;

		pthread_mutex_lock(&clients_mutex);
		struct client_list* node = malloc((sizeof(*node)));
		node->cli = cli;
		node->next = clients;
		node->back = NULL;
		if (clients)
			clients->back = node;
		clients = node;
		pthread_mutex_unlock(&clients_mutex);
	}
}
