#ifndef DLIST_H
#define DLIST_H

/*
 * Circular doubly linked list for general use.
 *
 * This implementation allows for efficient manipulation of double linked
 * structures where elements can be dynamically added or removed. Direct
 * manipulation of previous and next entries allows for optimized code when the
 * context of operations is already known.
 */

typedef struct double_linked_node {
  struct double_linked_node *next, *prev;
} double_linked_node_t;

/**
 * double_linked_init - Initialize a double_linked_node structure
 * @list: double_linked_node structure to be initialized.
 *
 * Initializes the double_linked_node to point to itself, representing an empty
 * list.
 */
static inline void double_linked_init(struct double_linked_node *list) {
  list->next = list;
  list->prev = list;
}

static inline void __double_linked_add(struct double_linked_node *new_node,
                                       struct double_linked_node *prev,
                                       struct double_linked_node *next) {
  next->prev = new_node;
  new_node->next = next;
  new_node->prev = prev;
  prev->next = new_node;
}

/**
 * double_linked_add_after - add a new entry after the specified node
 * @new_node: new entry to be added
 * @node: node to add it after
 *
 * Insert a new entry immediately after the specified node.
 * This is useful for quickly expanding the list.
 */
static inline void double_linked_add_after(struct double_linked_node *new_node,
                                           struct double_linked_node *node) {
  __double_linked_add(new_node, node, node->next);
}

/**
 * double_linked_add_before - add a new entry before the specified node
 * @new_node: new entry to be added
 * @node: node to add it before
 *
 * Insert a new entry immediately before the specified node.
 * This is useful for maintaining order in the list.
 */
static inline void double_linked_add_before(struct double_linked_node *new_node,
                                            struct double_linked_node *node) {
  __double_linked_add(new_node, node->prev, node);
}

/*
 * Remove an entry by making the prev/next entries
 * point to each other.
 */
static inline void __double_linked_remove(struct double_linked_node *prev,
                                          struct double_linked_node *next) {
  next->prev = prev;
  prev->next = next;
}

/**
 * double_linked_remove - remove a node from its list
 * @node: the node to be removed
 *
 * This function unlinks the specified node from the double linked list
 * by adjusting the pointers of its previous and next nodes. It does not
 * nullify the node's next and prev pointers.
 */
static inline void double_linked_remove(struct double_linked_node *node) {
  __double_linked_remove(node->prev, node->next);
}

/**
 * double_linked_is_head - tests whether @node is the head of the list
 * @node: the entry to test
 * @head: the head of the list
 */
static inline int double_linked_is_head(const struct double_linked_node *node,
                                        const struct double_linked_node *head) {
  return node == head;
}

/**
 * double_linked_is_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int
double_linked_is_empty(const struct double_linked_node *head) {
  return head->next == head;
}

/**
 * double_linked_for_each - iterate over a list
 * @cur: the &struct double_linked_node to use as a loop cursor.
 * @head: the head for your list.
 */
#define double_linked_for_each(cur, head)                                      \
  for (cur = (head)->next; !double_linked_is_head(cur, (head)); cur = cur->next)

#endif /* DLIST_H */
