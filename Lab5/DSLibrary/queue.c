#include "queue.h"

void init_queue(Queue* q) {
    init_list(&q->list);
}

void enqueue(Queue* queue, void* data) {
    insert_at_end(&queue->list.head, data);
}

void* dequeue(Queue* queue) {
    if (queue->list.head == NULL) {
        return NULL; // Return NULL if the queue is empty.
    }

    // Get the data from the head of the list to return it
    void* data = queue->list.head->data;

    // Call delete_node to remove the head node properly
    delete_node(&queue->list.head, queue->list.head);

    return data; // Return the data from the removed node
}

int queue_element_count(Queue* queue) {
    int cnt = 0;
    doubly_linked_list_node_t* node = queue->list.head;
    while (node != NULL) {
        cnt++;
        node = node->next;
    }
    return cnt;
}
