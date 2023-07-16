#ifndef __MSG_QUEUE_H__
#define __MSG_QUEUE_H__

#include "../msg.h"

#include "client_op.h"

struct msg_node {
	struct msg base;
	struct client cli;
	struct msg_node* next;
};

struct msg_queue {
	struct msg_node* head;
};

void msg_queue_push(struct msg_queue* queue, const struct msg* m, struct client* cli);
struct msg msg_queue_pop(struct msg_queue* queue);
int msg_queue_is_empty(struct msg_queue* queue);

#endif // __MSG_QUEUE_H__
