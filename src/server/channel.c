#include <pthread.h>
#include <stdlib.h>

#include "../utils.h"

#include "channel.h"
#include "client_op.h"
#include "msg_queue.h"

struct channel channel_create(const struct sv name, struct client* cli) {
	struct channel result  = { 0 };

	memcpy(result.name, name.base, MIN(name.len, 200));

	pthread_mutex_init(&result.clients_mutex, NULL);
	pthread_mutex_init(&result.queue_mutex, NULL);
	pthread_cond_init(&result.queue_cond, NULL);

	*cli = client_list_add(&result.clients, cli);
	cli->is_admin = true;

	return result;
}

int channel_check_name(struct channel* ch) {
	struct sv name = sv_from_cstr(ch->name);

	if (sv_contains(name, '#') != 0 && sv_contains(name, '&') != 0)
		return -1;

	if (sv_contains(name, ' ') != -1)
		return -1;

	if (sv_contains(name, 7) != -1)
		return -1;

	if (sv_contains(name, ',') != -1)
		return -1;

	return 0;
}

void channel_add_client(struct channel* ch, struct client* cli) {
	pthread_mutex_lock(&ch->clients_mutex);

	*cli = client_list_add(&ch->clients, cli);

	pthread_mutex_unlock(&ch->clients_mutex);
}

void channel_get_addr_by_username(char* addr, struct channel* ch, const char* username) {
	pthread_mutex_lock(&ch->clients_mutex);

	struct client* peer = client_list_find_by_nickname(&ch->clients, username);
	if (!peer)
		return;
	snprintf(addr, INET_ADDRSTRLEN+16, "%s %d", peer->soc.ip, peer->soc.port);

	pthread_mutex_unlock(&ch->clients_mutex);
}

void channel_mute_by_username(struct channel* ch, const char* username) {
	pthread_mutex_lock(&ch->clients_mutex);

	struct client* peer = client_list_find_by_nickname(&ch->clients, username);
	if (!peer)
		return;
	peer->is_mute = true;

	pthread_mutex_unlock(&ch->clients_mutex);
}

void channel_unmute_by_username(struct channel* ch, const char* username) {
	pthread_mutex_lock(&ch->clients_mutex);

	struct client* peer = client_list_find_by_nickname(&ch->clients, username);
	if (!peer)
		return;
	peer->is_mute = false;

	pthread_mutex_unlock(&ch->clients_mutex);
}

void channel_remove_client_by_username(struct channel* ch, const char* username) {
	pthread_mutex_lock(&ch->clients_mutex);

	client_list_remove_by_nickname(&ch->clients, username);

	pthread_mutex_unlock(&ch->clients_mutex);
}

bool channel_is_mute_by_username(struct channel* ch, const char* username) {
	pthread_mutex_lock(&ch->clients_mutex);

	struct client* peer = client_list_find_by_nickname(&ch->clients, username);
	if (!peer)
		return true;
	
	bool res = peer->is_mute;

	pthread_mutex_unlock(&ch->clients_mutex);

	return res;
}

void channel_add_msg(struct channel* ch, struct msg* msg, struct client* cli) {
	pthread_mutex_lock(&ch->queue_mutex);

	msg_queue_push(&ch->msgs, msg, cli);
	pthread_cond_signal(&ch->queue_cond);

	pthread_mutex_unlock(&ch->queue_mutex);
}

void channel_deliver_msg(struct channel* ch) {
	pthread_mutex_lock(&ch->queue_mutex);

	while (msg_queue_is_empty(&ch->msgs)) {
		pthread_cond_wait(&ch->queue_cond, &ch->queue_mutex);
	}

	struct msg msg = msg_queue_pop(&ch->msgs);

	client_list_send(&ch->clients, &msg);

	pthread_mutex_unlock(&ch->queue_mutex);
}


struct channel_list channel_list_create(void) {
	struct channel_list result = { 0 };
	pthread_mutex_init(&result.list_mutex, NULL);

	return result;
}

void* channel_handle(void* arg) {
	struct channel* ch = (struct channel*)arg;

	while(1) {
		pthread_mutex_lock(&ch->clients_mutex);
		if (client_list_is_empty(&ch->clients))
			break;
		pthread_mutex_unlock(&ch->clients_mutex);
		channel_deliver_msg(ch);
	}

	return NULL;
}

struct channel* channel_list_add(struct channel_list* ch_list, struct channel* ch) {
	if (channel_check_name(ch))
		return NULL;

	pthread_mutex_lock(&ch_list->list_mutex);

	struct channel_node* node = malloc(sizeof(struct channel_node));

	node->base = *ch;

	node->next = ch_list->head;
	ch_list->head = node;

	pthread_mutex_unlock(&ch_list->list_mutex);

	pthread_t id;
	EXIT_ON_ERROR(pthread_create(&id, NULL, &channel_handle, &node->base));
	pthread_detach(id);

	return &node->base;
}

struct channel* channel_list_find(struct channel_list* ch_list, struct sv name) {
	for (struct channel_node* node = ch_list->head; node; node = node->next) {
		if (!sv_comp(sv_from_cstr(node->base.name), name)) {
			return &node->base;
		}
	}

	return NULL;
}

void channel_list_remove_by_name(struct channel_list* ch_list, char* name) {
	pthread_mutex_lock(&ch_list->list_mutex);

	for (struct channel_node* node = ch_list->head; node; node = node->next) {
		if (!strcmp(node->base.name, name)) {
			node->back->next = node->next;
			node->next->back = node->back;
			free(node);
			return;
		}
	}

	pthread_mutex_unlock(&ch_list->list_mutex);
}
