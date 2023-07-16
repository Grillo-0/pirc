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
	cmd_str[CMD_JOIN] = "JOIN";
	cmd_str[CMD_NICKNAME] = "NICK";
	cmd_str[CMD_INVALID] = "INVALID";
	cmd_str[CMD_KICK] = "KICK";
	cmd_str[CMD_WHOIS] = "WHOIS";
	cmd_str[CMD_MUTE] = "MUTE";
	cmd_str[CMD_UNMUTE] = "UNMUTE";

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
	if (!strcmp(cmd, "JOIN"))
		return CMD_JOIN;
	if (!strcmp(cmd, "NICK"))
		return CMD_NICKNAME;
	if (!strcmp(cmd, "KICK"))
		return CMD_KICK;
	if (!strcmp(cmd, "WHOIS"))
		return CMD_WHOIS;
	if (!strcmp(cmd, "MUTE"))
		return CMD_MUTE;
	if (!strcmp(cmd, "UNMUTE"))
		return CMD_UNMUTE;
	return CMD_INVALID;
}

static void parse_params(struct msg* msg, struct sv buf) {
	msg->params_size = 0;
	msg->params = NULL;

	if (!sv_comp(buf, SV("\r\n")))
		return;

	buf = sv_chop_by_delim(&buf, '\r');

	while (buf.len != 0) {
		msg->params = realloc(msg->params, msg->params_size + 1);
		msg->params[msg->params_size] = sv_chop_by_delim(&buf, ':');
		msg->params_size++;
	}
}

static void parse_raw_msg(struct msg* msg) {

	struct sv buf_sv = sv_from_cstr(msg->raw);

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

void add_param(char** buf, char* param) {
	memcpy(*buf, param, strlen(param));
	*buf += strlen(param);
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
		add_param(&raw_ptr, m);
		break;
	case CMD_PING:
	case CMD_PONG:
	case CMD_CONNECT:
		break;
	case CMD_JOIN:
		msg->params_size = 1;
		char* channel = va_arg(args, char*);
		add_param(&raw_ptr, channel);
		break;
	case CMD_NICKNAME:
		break;
	case CMD_KICK:
		msg->params_size = 1;
		char* username = va_arg(args, char*);
		add_param(&raw_ptr, username);
		break;
	case CMD_WHOIS:
		msg->params_size = 1;
		char* user = va_arg(args, char*);
		add_param(&raw_ptr, user);
		break;
	case CMD_MUTE:
		msg->params_size = 1;
		char* user_mute = va_arg(args, char*);
		add_param(&raw_ptr, user_mute);
		break;
	case CMD_UNMUTE:
		msg->params_size = 1;
		char* user_unmute = va_arg(args, char*);
		add_param(&raw_ptr, user_unmute);
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

	parse_raw_msg(msg);
}

void msg_free(struct msg* msg) {
	memset(msg->raw, 0, MSG_SIZE);

	msg->cmd = CMD_INVALID;
	if (msg->params_size) {
		msg->params_size = 0;
		free(msg->params);
	}
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

	memcpy(msg->raw, buf, MSG_SIZE);
	parse_raw_msg(msg);

	return 0;
}
