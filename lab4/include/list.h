#include "mini_uart.h"
struct list_head {
    struct list_head *prev, *next;
};

#define LIST_HEAD(head) struct list_head head = {&(head), &(head)}

/*
 * INIT_LIST_HEAD() - Initialize empty list head
 * @head: pointer to list head
 */
static inline void INIT_LIST_HEAD(struct list_head *head)
{
    head->next = head;
    head->prev = head;
}

/*
 * list_add() - add a node in the head of doubly linked list
 * @node: pointer to added node
 * @prev: pointer to previous node
 * @next: pointer to next node
 */
static inline void list_add(struct list_head *node, struct list_head *prev, struct list_head *next)
{
    prev->next = node;
    next->prev = node;
    node->prev = prev;
    node->next = next;
}

/*
 * list_add_tail() - add a node in the tail of doubly linked list
 * @node: pointer to added node
 * @prev: pointer to previous node
 * @next: pointer to next node
 */

static inline void list_add_tail(struct list_head *node, struct list_head *head)
{
    // printf("node: %8x\n", node);
    // printf("head: %8x\n", head);
    list_add(node, head->prev, head);
}

/*
 * list_del() - Remove a list node from the list
 * @node: pointer to the node
 */ 
static inline void list_del(struct list_head *node)
{
    struct list_head *next = node->next;
    struct list_head *prev = node->prev;

    next->prev = prev;
    prev->next = next;
}

/*
 * node_is_first() - Check if the node is the first node in the list
 * @node: pointer to the node
 * @head: pointer to the head of the list
 *
 * Return: 0 - node is not the first node of the list !0 - node is the first node of the list.
 */
static inline int node_is_first(const struct list_head *node, const struct list_head *head)
{
    return (node == head->next);
}

/*
 * list_is_empty() - Check if the list is empty or not
 * @head: pointer to the head of the list
 *
 * Return: 1 - the list is empty, only contains the head of list.
 */
static inline int list_is_empty(const struct list_head * head)
{
    return (head->next == head);
}