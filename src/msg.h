#ifndef __MSG_H__
#define __MSG_H__

#include <stddef.h>

#include "socket.h"
#include "sv.h"

#define MSG_SIZE 4096

enum msg_cmd {
	CMD_MSG,
	CMD_PING,
	CMD_PONG,
	CMD_CONNECT,
	CMD_JOIN,
	CMD_NICKNAME,
	CMD_KICK,
	CMD_WHOIS,
	CMD_MUTE,
	CMD_UNMUTE,
	CMD_INVALID,
	CMD_SIZE,
};

struct msg {
	char raw[MSG_SIZE];

	struct sv prefix;
	enum msg_cmd cmd;
	size_t params_size;
	struct sv* params;
};

void msg_create(struct msg* msg, char* prefix, enum msg_cmd cmd, ...);
void msg_free(struct msg* msg);
void msg_send(struct socket* soc, struct msg* msg);
int msg_recv(struct socket* soc, struct msg* msg);

#endif // __MSG_H__
