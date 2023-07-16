#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../utils.h"
#include "../msg.h"

#include "channel.h"
#include "msg_queue.h"
#include "client_op.h"

struct channel_list channels;

void handle_msg_on_lobby(struct client* cli, struct msg* msg) {

	// If a client is trying to send some message before connecting just
	// ignore it
	if (!*cli->nickname && msg->cmd != CMD_CONNECT)
		return;

	switch (msg->cmd) {
	case CMD_PING:;
		struct msg res;
		msg_create(&res, NULL, CMD_PONG);
		msg_send(&cli->soc, &res);
		msg_free(&res);
		break;
	case CMD_CONNECT:
		memcpy(cli->nickname, msg->prefix.base, MIN(msg->prefix.len, 50));
		break;
	case CMD_JOIN:
		if((cli->curr_ch = channel_list_find(&channels, msg->params[0]))) {
			channel_add_client(cli->curr_ch, cli);
			break;
		}

		struct channel ch = channel_create(msg->params[0], cli);
		cli->curr_ch = channel_list_add(&channels, &ch);
		break;
	case CMD_NICKNAME:
		memcpy(cli->nickname, msg->prefix.base, MIN(msg->prefix.len, 50));
		break;
	case CMD_INVALID:
	case CMD_SIZE:
	case CMD_MSG:
	case CMD_PONG:
	default:
		break;
	}
}

void handle_msg(struct client* cli, struct msg* msg) {
	char username[51] = { 0 };
	struct msg res;
	switch (msg->cmd) {
	case CMD_MSG:
		if (!cli->curr_ch)
			return;

		if (channel_is_mute_by_username(cli->curr_ch, cli->nickname))
			return;

		channel_add_msg(cli->curr_ch, msg, cli);
		return;
	case CMD_PING:;
		msg_create(&res, NULL, CMD_PONG);
		msg_send(&cli->soc, &res);
		msg_free(&res);
		break;
	case CMD_NICKNAME:
		memcpy(cli->nickname, msg->prefix.base, MIN(msg->prefix.len, 50));
		break;
	case CMD_KICK:;
		if (!cli->is_admin)
			break;
		memcpy(username, msg->params[0].base, MIN(msg->params[0].len, 50));
		channel_remove_client_by_username(cli->curr_ch, username);
		break;
	case CMD_WHOIS:;
		if (!cli->is_admin)
			break;
		memcpy(username, msg->params[0].base, MIN(msg->params[0].len, 50));
		
		char res_msg[INET_ADDRSTRLEN+16] = {0};

		channel_get_addr_by_username(res_msg, cli->curr_ch, username);

		msg_create(&res, NULL, CMD_MSG, res_msg);
		msg_send(&cli->soc, &res);
		msg_free(&res);
		break;
	case CMD_MUTE:;
		if (!cli->is_admin)
			break;

		memcpy(username, msg->params[0].base, MIN(msg->params[0].len, 50));
		channel_mute_by_username(cli->curr_ch, username);
		break;
	case CMD_UNMUTE:;
		if (!cli->is_admin)
			break;

		memcpy(username, msg->params[0].base, MIN(msg->params[0].len, 50));
		channel_unmute_by_username(cli->curr_ch, username);
		break;
	case CMD_CONNECT:
	case CMD_JOIN:
	case CMD_PONG:
	case CMD_INVALID:
	case CMD_SIZE:
	default:
		break;
	}
}

void* handle_client(void* arg) {
	struct client cli = *(struct client*)arg;

	while (1) {
		struct msg received;
		if (msg_recv(&cli.soc, &received))
			return NULL;

		handle_msg_on_lobby(&cli, &received);
		msg_free(&received);
		if (cli.curr_ch)
			break;
	}

	while (1) {
		struct msg received;
		if (msg_recv(&cli.soc, &received))
			return NULL;

		handle_msg(&cli, &received);
		msg_free(&received);
	}

	return NULL;
}

struct socket serv_soc;
void handle_exit(int signum) {
	(void)signum;

	exit(EXIT_SUCCESS);
}

int main(void) {
	channels = channel_list_create();

	socket_create(&serv_soc);
	int reuse = 1;
	EXIT_ON_ERROR(setsockopt(serv_soc.fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)));
	socket_bind(&serv_soc, 42069);

	signal(SIGINT, handle_exit);

	EXIT_ON_ERROR(socket_listen(&serv_soc));

	while (1) {
		struct socket client_soc;
		pthread_t client_id;
		EXIT_ON_ERROR(socket_accept(&serv_soc, &client_soc));

		struct client cli = client_create(&client_soc);
		EXIT_ON_ERROR(pthread_create(&client_id, NULL, handle_client, &cli));
		pthread_detach(client_id);
	}
}
