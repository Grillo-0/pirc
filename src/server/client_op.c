#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "channel.h"
#include "client_op.h"

struct client client_create(struct socket* soc) {
	struct client result  = {
		.soc = *soc,
		.is_admin = false,
		.is_mute = false,
	};

	return result;
}


struct client_list client_list_create(void) {
	struct client_list result = { 
		.head = NULL
	};

	return result;
}

struct client client_list_add(struct client_list* list, const struct client* cli) {
	struct client_node* node = malloc(sizeof(struct client_node));

	node->cli = *cli;

	if (list->head)
		list->head->back = node;

	node->next = list->head;
	list->head = node;

	return node->cli;
}

struct client* client_list_find_by_nickname(struct client_list* list, const char* username) {
	for (struct client_node* node = list->head; node; node = node->next) {
		if (!strcmp(node->cli.nickname, username)) {
			return &node->cli;
		}
	}

	return NULL;
}

void client_list_remove_by_nickname(struct client_list* list, const char* username) {
	for (struct client_node* node = list->head; node; node = node->next) {
		if (!strcmp(node->cli.nickname, username)) {
			close(node->cli.soc.fd);
			node->cli.curr_ch = NULL;

			if (node->back)
				node->back->next = node->next;

			if (node->next)
				node->next->back = node->back;

			if (node == list->head)
				list->head = list->head->next;


			free(node);
			return;
		}
	}
}

int client_list_is_empty(struct client_list* list) {
	return !list->head;
}

void client_list_send(struct client_list* list, struct msg* msg) {
	for (struct client_node* node = list->head; node; node = node->next)
		msg_send(&node->cli.soc, msg);
}
