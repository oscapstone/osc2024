#ifndef LIST_H
#define LIST_H

#include <stddef.h>

/*
 * Circular doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 *
 * https://github.com/torvalds/linux/blob/master/include/linux/list.h
 * https://elixir.bootlin.com/linux/latest/source/scripts/kconfig/list.h#L24
 */

typedef struct list_head {
	struct list_head *next, *prev;
}list_head_t;

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

/**
 * INIT_LIST_HEAD - Initialize a list_head structure
 * @list: list_head structure to be initialized.
 *
 * Initializes the list_head to point to itself.  If it is a list header,
 * the result is an empty list.
 */
static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static inline void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void list_del_entry(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}


/**
 * list_is_head - tests whether @list is the list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int list_is_head(const struct list_head *list, const struct list_head *head)
{
	return list == head;
}


/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; !list_is_head(pos, (head)); pos = pos->next)

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int list_size(const struct list_head *head)
{
	list_head_t *pos;
	int i= 0;
	list_for_each(pos, head)
	{
		i++;
	}
	return i;
}

/* "typeof" is a GNU extension.
 * Reference: https://gcc.gnu.org/onlinedocs/gcc/Typeof.html
 */
#if defined(__GNUC__)
#define __LIST_HAVE_TYPEOF 1
#endif /* defined(__GNUC__) */

/**
 * container_of() - Calculate address of object that contains address ptr
 * @ptr: pointer to member variable
 * @type: type of the structure containing ptr
 * @member: name of the member variable in struct @type
 *
 * Return: @type pointer of object containing ptr
 */
#ifndef container_of
#ifdef __LIST_HAVE_TYPEOF
#define container_of(ptr, type, member)                            \
    __extension__({                                                \
        const __typeof__(((type *) 0)->member) *__pmember = (ptr); \
        (type *) ((char *) __pmember - offsetof(type, member));    \
    })
#else /* __LIST_HAVE_TYPEOF */
#define container_of(ptr, type, member) \
    ((type *) ((char *) (ptr) -offsetof(type, member)))
#endif /* __LIST_HAVE_TYPEOF */
#endif /* container_of */

/**
 * list_entry() - Calculate address of entry that contains list node
 * @node: pointer to list node
 * @type: type of the entry containing the list node
 * @member: name of the list_head member variable in struct @type
 *
 * Return: @type pointer of entry containing node
 */
#define list_entry(node, type, member) container_of(node, type, member)

/**
 * list_first_entry() - get first entry of the list
 * @head: pointer to the head of the list
 * @type: type of the entry containing the list node
 * @member: name of the list_head member variable in struct @type
 *
 * Return: @type pointer of first entry in list
 */
#define list_first_entry(head, type, member) \
    list_entry((head)->next, type, member)

/**
 * list_for_each_entry - iterate over list entries
 * @entry: pointer used as iterator
 * @head: pointer to the head of the list
 * @member: name of the list_head member variable in struct type of @entry
 *
 * The nodes and the head of the list must be kept unmodified while
 * iterating through it. Any modifications to the the list will cause undefined
 * behavior.
 *
 * FIXME: remove dependency of __typeof__ extension
 */
#ifdef __LIST_HAVE_TYPEOF
#define list_for_each_entry(entry, head, member)                       \
    for (entry = list_entry((head)->next, __typeof__(*entry), member); \
         &entry->member != (head);                                     \
         entry = list_entry(entry->member.next, __typeof__(*entry), member))
#endif /* __LIST_HAVE_TYPEOF */

/**
 * iter_for_each_entry - iterate over list entries from @iter
 * @entry: pointer used as iterator
 * @iter: pointer to the start of this iteration
 * @head: pointer to the head of the list
 * @member: name of the list_head member variable in struct type of @entry
 *
 * FIXME: remove dependency of __typeof__ extension
 */
#ifdef __LIST_HAVE_TYPEOF
#define iter_for_each_entry(entry, iter, head, member)                 \
    for (entry = list_entry(iter, __typeof__(*entry), member);         \
         &entry->member != (head);                                     \
         entry = list_entry(entry->member.next, __typeof__(*entry), member))
#endif /* __LIST_HAVE_TYPEOF */

#endif
