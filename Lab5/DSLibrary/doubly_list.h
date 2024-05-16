#ifndef _DOUBLY_LIST_H_
#define _DOUBLY_LIST_H_

typedef struct doubly_linked_list_node {
    struct doubly_linked_list_node* next;
    struct doubly_linked_list_node* prev;
    void*   data;
} doubly_linked_list_node_t;

typedef struct doubly_linked_list {
    doubly_linked_list_node_t* head;
} doubly_linked_list_t;

void init_list(doubly_linked_list_t* list);
void insert_at_start(doubly_linked_list_node_t** head, void* data);
void insert_at_end(doubly_linked_list_node_t** head, void* data);
void insert_before(doubly_linked_list_node_t **head, doubly_linked_list_node_t* fixNode, void* data);
void delete_node(doubly_linked_list_node_t **head, doubly_linked_list_node_t *nodeToDelete);

#endif