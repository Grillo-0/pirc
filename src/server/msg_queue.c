#include <stdlib.h>
#include <string.h>

#include "msg_queue.h"

void msg_queue_push(struct msg_queue* queue, const struct msg* m, struct client* cli) {
	struct msg_node* node = malloc(sizeof(struct msg_node));

	node->base = *m;
	node->cli = *cli;

	node->next = queue->head;
	queue->head = node;
}

struct msg msg_queue_pop(struct msg_queue* queue) {
	struct msg result = queue->head->base; 

	struct msg_node* node = queue->head;
	queue->head = queue->head->next;
	free(node);

	return result;
}

int msg_queue_is_empty(struct msg_queue* queue) {
	return !queue->head;
}
