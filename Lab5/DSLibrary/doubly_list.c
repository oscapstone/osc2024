#include "doubly_list.h"
#include "../peripherals/utils.h"
#include "../kernel/dynamic_alloc.h"

void init_list(doubly_linked_list_t* list) {
    list->head = NULL;
}

void insert_at_start(doubly_linked_list_node_t** head, void* data) {
    doubly_linked_list_node_t* temp = (doubly_linked_list_node_t*)dynamic_alloc(sizeof(doubly_linked_list_node_t));
    if (!temp) return;  // Handle allocation failure

    temp->data = data;
    temp->next = *head;
    temp->prev = NULL;

    if (*head != NULL) {
        (*head)->prev = temp;
    }

    *head = temp;
}

void insert_at_end(doubly_linked_list_node_t** head, void* data) {
    doubly_linked_list_node_t* temp = (doubly_linked_list_node_t*)dynamic_alloc(sizeof(doubly_linked_list_node_t));
    if (!temp) return;  // Handle allocation failure

    temp->data = data;
    temp->next = NULL;

    if (*head == NULL) {
        temp->prev = NULL;
        *head = temp;
        return;
    }

    doubly_linked_list_node_t* ptr = *head;
    while (ptr->next != NULL) {
        ptr = ptr->next;
    }

    ptr->next = temp;
    temp->prev = ptr;
}

void insert_before(doubly_linked_list_node_t** head, doubly_linked_list_node_t* fixNode, void* data) {
    if (!head || !fixNode) return;

    doubly_linked_list_node_t* temp = (doubly_linked_list_node_t*)dynamic_alloc(sizeof(doubly_linked_list_node_t));
    if (!temp) return;  // Handle allocation failure

    temp->data = data;

    if (*head == fixNode) {
        temp->next = *head;
        (*head)->prev = temp;
        *head = temp;
        return;
    }

    doubly_linked_list_node_t* ptr = *head;
    while (ptr != NULL && ptr->next != fixNode) {
        ptr = ptr->next;
    }

    if (ptr == NULL) return;  // fixNode not found in the list

    temp->next = fixNode;
    temp->prev = ptr;
    ptr->next = temp;
    fixNode->prev = temp;
}

void delete_node(doubly_linked_list_node_t** head, doubly_linked_list_node_t* nodeToDelete) {
    if (*head == NULL || nodeToDelete == NULL) {
        return;
    }

    if (*head == nodeToDelete) {
        *head = nodeToDelete->next;
    }

    if (nodeToDelete->next != NULL) {
        nodeToDelete->next->prev = nodeToDelete->prev;
    }

    if (nodeToDelete->prev != NULL) {
        nodeToDelete->prev->next = nodeToDelete->next;
    }

    dynamic_free((uint64_t)nodeToDelete);
}
