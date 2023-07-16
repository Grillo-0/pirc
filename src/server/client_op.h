#ifndef __CLIENT_OP_H_
#define __CLIENT_OP_H_

#include <stdbool.h>

#include "../msg.h"

#include "../socket.h"

struct channel;

struct client {
	struct socket soc;
	char nickname[51];
	struct channel* curr_ch;
	bool is_admin;
	bool is_mute;
};

struct client_node {
	struct client cli;
	struct client_node* next;
	struct client_node* back;
};

struct client_list {
	struct client_node* head;
};

struct client client_create(struct socket* soc);
struct client_list client_list_create(void);
struct client client_list_add(struct client_list* list, const struct client* cli);
struct client* client_list_find_by_nickname(struct client_list* list, const char* username);
void client_list_remove_by_nickname(struct client_list* list, const char* username);
int client_list_is_empty(struct client_list* list);
void client_list_send(struct client_list* list, struct msg* msg);

#endif // __CLIENT_OP_H_
