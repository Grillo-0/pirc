#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include <pthread.h>

#include "../sv.h"

#include "client_op.h"
#include "msg_queue.h"

struct channel {
	char name[201];

	pthread_mutex_t clients_mutex;
	struct client_list clients;

	pthread_mutex_t queue_mutex;
	pthread_cond_t queue_cond;
	struct msg_queue msgs;
};

struct channel channel_create(const struct sv name, struct client* cli);
int channel_check_name(struct channel* ch);
void channel_add_client(struct channel* ch, struct client* cli);
void channel_get_addr_by_username(char* addr, struct channel* ch, const char* username);
void channel_mute_by_username(struct channel* ch, const char* username);
void channel_unmute_by_username(struct channel* ch, const char* username);
bool channel_is_mute_by_username(struct channel* ch, const char* username);
void channel_remove_client_by_username(struct channel* ch, const char* username);
void channel_add_msg(struct channel* ch, struct msg* msg, struct client* cli);
void channel_deliver_msg(struct channel* ch);

struct channel_node {
	struct channel base;
	struct channel_node* next;
	struct channel_node* back;
};

struct channel_list {
	pthread_mutex_t list_mutex;
	struct channel_node* head;
};

struct channel_list channel_list_create(void);
struct channel* channel_list_add(struct channel_list* ch_list, struct channel* ch);
struct channel* channel_list_find(struct channel_list* ch_list, struct sv name);
void channel_list_remove_by_name(struct channel_list* ch_list, char* name);

#endif // __CHANNEL_H__
