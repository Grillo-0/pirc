#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "../socket.h"
#include "../utils.h"
#include "../msg.h"

void handle_msg(struct msg* msg) {
	switch (msg->cmd) {
	case CMD_MSG:
		if (msg->prefix.len == 0)
			printf("\r" "*SERVER*: " SV_FMT "\n>", SV_ARGS(msg->params[0]));
		else
			printf("\r" SV_FMT ": " SV_FMT "\n>", SV_ARGS(msg->prefix),
							      SV_ARGS(msg->params[0]));
		break;
	case CMD_PONG:
		if (msg->prefix.len == 0)
			printf("*SERVER*: PONG\n");
		break;
	case CMD_PING:
	case CMD_INVALID:
	case CMD_SIZE:
	default:
		break;
	}
}

void* recv_msgs(void* arg) {
	struct socket cli_soc = *(struct socket*)arg;
	while (1) {
		struct msg msg;
		if (msg_recv(&cli_soc, &msg)) {
			fprintf(stderr, "ERROR: Server is down\n");
			exit(-1);
		}
		handle_msg(&msg);
		fflush(stdout);
		msg_free(&msg);
	}
	return NULL;
}

struct client {
	char* nickname;
	struct socket soc;
};

struct client cli;

void handle_exit(int signum) {
	(void)signum;
	close(cli.soc.fd);
	exit(EXIT_SUCCESS);
}

void handle_input(char* input) {
	struct msg msg;

	if (input[0] != '/' && cli.nickname) {
		if (input[strlen(input) - 1] == '\n')
			input[strlen(input) - 1] = '\0';

		msg_create(&msg, cli.nickname, CMD_MSG, input);
		msg_send(&cli.soc, &msg);
		msg_free(&msg);
		return;
	}

	struct sv input_sv = sv_from_cstr(input);
	input_sv = sv_chop_by_delim(&input_sv, '\n');
	sv_chop_by_delim(&input_sv, '/');
	struct sv cmd = sv_chop_by_delim(&input_sv, ' ');

	if (!sv_comp(cmd, SV("quit"))) {
		close(cli.soc.fd);
		free(cli.nickname);
		cli.nickname = NULL;
		exit(EXIT_SUCCESS);
	}

	if (!cli.nickname && (sv_comp(cmd, SV("connect")))) {
		fprintf(stderr, "ERROR: client is not connected yet!\n");
		fprintf(stderr, "usage: /connect [nickname]@[remote]:[port]\n");
		return;
	}

	if (!sv_comp(cmd, SV("ping"))) {
		msg_create(&msg, cli.nickname, CMD_PING);
		msg_send(&cli.soc, &msg);
		msg_free(&msg);
	} else if (!sv_comp(cmd, SV("connect"))) {
		if (cli.nickname) {
			fprintf(stderr, "ERROR: client already connected!\n");
			return;
		}

		struct sv nickname_sv = sv_chop_by_delim(&input_sv, '@');
		if (!input_sv.len) {
			fprintf(stderr, "ERROR: no remote given!\n");
			fprintf(stderr, "usage: /connect [nickname]@[remote]:[port]\n");
			return;
		}
		struct sv remote_sv = sv_chop_by_delim(&input_sv, ':');
		if (!input_sv.len) {
			fprintf(stderr, "ERROR: no port given!\n");
			fprintf(stderr, "usage: /connect [nickname]@[remote]:[port]\n");
			return;
		}

		cli.nickname = sv_to_cstr(nickname_sv);
		char* remote = sv_to_cstr(remote_sv);
		char* port = sv_to_cstr(input_sv);

		socket_create(&cli.soc);
		if(socket_connect(&cli.soc, remote, atoi(port))) {
			fprintf(stderr, "ERROR: could not connect to server %s:%s!\n", remote, port);
			fprintf(stderr, "usage: /connect [nickname]@[remote]:[port]\n");
			free(cli.nickname);
			cli.nickname = NULL;
			free(remote);
			free(port);
			return;
		}
		msg_create(&msg, cli.nickname, CMD_CONNECT);

		signal(SIGINT, handle_exit);

		pthread_t id;
		pthread_create(&id, NULL, recv_msgs, &cli.soc);
		pthread_detach(id);

		msg_send(&cli.soc, &msg);
		msg_free(&msg);
		free(remote);
		free(port);
	} else if (!sv_comp(cmd, SV("join"))) {
		if (!input_sv.len) {
			fprintf(stderr, "ERROR: no channel given!\n");
			fprintf(stderr, "usage: /join [channel]\n");
			return;
		}
		char* ch_name = sv_to_cstr(sv_chop_by_delim(&input_sv, ' '));
		msg_create(&msg, cli.nickname, CMD_JOIN, ch_name);
		msg_send(&cli.soc, &msg);
		msg_free(&msg);
		free(ch_name);
	} else if (!sv_comp(cmd, SV("nickname"))) {
		if (!input_sv.len) {
			fprintf(stderr, "ERROR: no username given!\n");
			fprintf(stderr, "usage: /nickname [nickname]\n");
			return;
		}
		free(cli.nickname);
		cli.nickname = sv_to_cstr(input_sv);
		msg_create(&msg, cli.nickname, CMD_NICKNAME);
		msg_send(&cli.soc, &msg);
		msg_free(&msg);
	} else if (!sv_comp(cmd, SV("kick"))) {
		if (!input_sv.len) {
			fprintf(stderr, "ERROR: no username given!\n");
			fprintf(stderr, "usage: /kick [nickname]\n");
			return;
		}
		char* username = sv_to_cstr(input_sv);
		msg_create(&msg, cli.nickname, CMD_KICK, username);
		msg_send(&cli.soc, &msg);
		msg_free(&msg);
		free(username);
	} else if (!sv_comp(cmd, SV("whois"))) {
		if (!input_sv.len) {
			fprintf(stderr, "ERROR: no username given!\n");
			fprintf(stderr, "usage: /whois [nickname]\n");
			return;
		}
		char* username = sv_to_cstr(input_sv);
		msg_create(&msg, cli.nickname, CMD_WHOIS, username);
		msg_send(&cli.soc, &msg);
		msg_free(&msg);
		free(username);
	} else if (!sv_comp(cmd, SV("mute"))) {
		if (!input_sv.len) {
			fprintf(stderr, "ERROR: no username given!\n");
			fprintf(stderr, "usage: /mute [nickname]\n");
			return;
		}
		char* username = sv_to_cstr(input_sv);
		msg_create(&msg, cli.nickname, CMD_MUTE, username);
		msg_send(&cli.soc, &msg);
		msg_free(&msg);
		free(username);
	} else if (!sv_comp(cmd, SV("unmute"))) {
		if (!input_sv.len) {
			fprintf(stderr, "ERROR: no username given!\n");
			fprintf(stderr, "usage: /unmute [nickname]\n");
			return;
		}
		char* username = sv_to_cstr(input_sv);
		msg_create(&msg, cli.nickname, CMD_UNMUTE, username);
		msg_send(&cli.soc, &msg);
		msg_free(&msg);
		free(username);
	}
}

int main(void) {
	char buf[MSG_SIZE];

	while (1) {
		printf("> ");
		memset(buf, 0, MSG_SIZE);
		if (!fgets(buf, MSG_SIZE, stdin))
			return 0;

		handle_input(buf);
	}
}
