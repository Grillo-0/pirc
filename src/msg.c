#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "msg.h"
#include "socket.h"
#include "sv.h"
#include "utils.h"

static char* cmd2str(const enum msg_cmd cmd) {
	static char* cmd_str[CMD_SIZE];
	cmd_str[CMD_MSG] = "MSG";
	cmd_str[CMD_PING] = "PING";
	cmd_str[CMD_PONG] = "PONG";
	cmd_str[CMD_CONNECT] = "CON";
	cmd_str[CMD_INVALID] = "INVALID";

	return cmd_str[cmd];
}

static enum msg_cmd str2cmd(const char* cmd) {
	if (!strcmp(cmd, "MSG"))
		return CMD_MSG;
	if (!strcmp(cmd, "PING"))
		return CMD_PING;
	if (!strcmp(cmd, "PONG"))
		return CMD_PONG;
	if (!strcmp(cmd, "CON"))
		return CMD_CONNECT;
	return CMD_INVALID;
}

static void parse_params(struct msg* msg, struct sv buf) {
	msg->params_size = 0;
	msg->params = NULL;
	while (buf.len != 0) {
		msg->params = realloc(msg->params, msg->params_size + 1);
		msg->params[msg->params_size] = sv_chop_by_delim(&buf, ':');
		msg->params_size++;
	}
	msg->params[msg->params_size - 1].len -= 2;
}

static void parse_raw_msg(struct msg* msg, char* buf) {
	memcpy(msg, buf, MSG_SIZE);

	struct sv buf_sv = sv_from_cstr(buf);


	struct sv cmd = sv_chop_by_delim(&buf_sv, ' ');

	struct sv prefix = sv_chop_by_delim(&cmd, '|');
	if (cmd.len == 0) {
		cmd = prefix;
		prefix.base = NULL;
		prefix.len = 0;
	}

	msg->prefix = prefix;

	char cmd_str[32] = {0};
	memcpy(cmd_str, cmd.base, cmd.len);
	msg->cmd = str2cmd(cmd_str);

	parse_params(msg, buf_sv);
}


void msg_create(struct msg* msg, char* prefix, enum msg_cmd cmd, ...) {
	char* raw_ptr = msg->raw;
	if (prefix != NULL) {
		memcpy(raw_ptr, prefix, strlen(prefix));
		raw_ptr += strlen(prefix);
		*raw_ptr = '|';
		raw_ptr++;
	}

	char* cmd_str = cmd2str(cmd);
	memcpy(raw_ptr, cmd_str, strlen(cmd_str));
	raw_ptr += strlen(cmd_str);
	*raw_ptr = ' ';
	raw_ptr++;

	va_list args;
	va_start(args, cmd);
	switch (cmd) {
	case CMD_MSG:
		msg->params_size = 1;
		char* m = va_arg(args, char*);
		memcpy(raw_ptr, m, strlen(m));
		raw_ptr += strlen(m);
		break;
	case CMD_PING:
	case CMD_PONG:
	case CMD_CONNECT:
		break;
	case CMD_INVALID:
	case CMD_SIZE:
	default:
		EXIT_ON_ERROR(1);
		break;
	}
	va_end(args);

	*raw_ptr = '\r';
	raw_ptr++;
	*raw_ptr = '\n';

	parse_params(msg, sv_from_cstr(msg->raw + strlen(cmd_str) + 1));
}

void msg_free(struct msg* msg) {
	memset(msg->raw, 0, MSG_SIZE);

	msg->cmd = CMD_INVALID;
	msg->params_size = 0;
	free(msg->params);
}

void msg_send(struct socket* soc, struct msg* msg) {
	EXIT_ON_ERROR(socket_send(soc, msg->raw, MSG_SIZE));
}

int msg_recv(struct socket* soc, struct msg* msg) {
	char buf[MSG_SIZE] = {0};

	size_t msg_len = MSG_SIZE;

	if(socket_recv(soc, buf, &msg_len))
		return -1;

	if (!msg_len)
		return -1;

	bool finished = false;
	while(!finished) {
		for(size_t i = 0; i < msg_len - 1; i++) {
			if (buf[i] == '\r' && buf[i+1] == '\n') {
				finished = true;
				break;
			}
		}

		msg_len = MSG_SIZE - msg_len;
		if (msg_len == 0)
			break;
		socket_recv(soc, buf + msg_len, &msg_len);
	}

	if (!finished) {
		errno = EBADMSG;
		return -1;
	}

	parse_raw_msg(msg, buf);

	return 0;
}
