#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "../pirc.h"
#include "../utils.h"
#include "../socket.h"
#include "../msg.h"

int main(int argc, char* argv[]) {
	bool is_first = true;

	if (argc == 3) {
		is_first = false;
	} else if (argc != 1) {
		fprintf(stderr, "ERROR: wrong number of arguments\n");
		fprintf(stderr, "usage: %s [peer-host] [peer-port]\n", argv[0]);
		exit(-1);
	}

	struct socket soc;
	socket_create(&soc);

	socket_bind(&soc, 0);
	printf("running on port: %d\n", soc.port);

	struct socket com_soc;
	char buf[MSG_SIZE];
	struct msg msg;

	if (is_first) {
		EXIT_ON_ERROR(socket_listen(&soc));
		EXIT_ON_ERROR(socket_accept(&soc, &com_soc));

		EXIT_ON_ERROR(msg_recv(&com_soc, &msg));
		printf("received: "SV_FMT"\n", SV_ARGS(msg.params[0]));
	} else {
		EXIT_ON_ERROR(socket_connect(&soc, argv[1], atoi(argv[2])));

		printf("> ");
		memset(buf, 0, MSG_SIZE);
		fgets(buf, MSG_SIZE, stdin);

		msg_create(&msg, NULL, CMD_MSG, buf);
		msg_send(&soc, &msg);
		msg_free(&msg);
	}

	while(1) {
		if (is_first) {
			printf("> ");
			memset(buf, 0, MSG_SIZE);
			fgets(buf, MSG_SIZE, stdin);

			if(buf[strlen(buf) - 1] == '\n')
				buf[strlen(buf) - 1] = 0;

			msg_create(&msg, NULL, CMD_MSG, buf);
			msg_send(&com_soc, &msg);
			msg_free(&msg);

			EXIT_ON_ERROR(msg_recv(&com_soc, &msg));
			printf("received: "SV_FMT"\n", SV_ARGS(msg.params[0]));
			msg_free(&msg);
		} else {
			EXIT_ON_ERROR(msg_recv(&soc, &msg));
			printf("received: "SV_FMT"\n", SV_ARGS(msg.params[0]));
			msg_free(&msg);

			printf("> ");
			memset(buf, 0, MSG_SIZE);
			fgets(buf, MSG_SIZE, stdin);

			msg_create(&msg, NULL, CMD_MSG, buf);
			msg_send(&soc, &msg);
			msg_free(&msg);
		}
	}

	return 0;
}

