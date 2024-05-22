#ifndef LIST_H
#define LIST_H

#include "malloc.h"

typedef struct list_head {
    struct list_head *next, *prev;
} list_head_t;

extern list_head_t task_list;
extern list_head_t *timer_event_list;

#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define LIST_HEAD(name) list_head_t name = LIST_HEAD_INIT(name)

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)
#define list_for_each_prev(pos, head) \
  for (pos = (head)->prev; prefetch(pos->prev), pos != (head); pos = pos->prev)

#define offsetof(TYPE, MEMBER) ((unsigned int) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
               (type *)( (char *)__mptr - offsetof(type,member) );})

#define list_entry(ptr,type,member)     \
    container_of(ptr, type, member)

void INIT_LIST_HEAD(list_head_t *head);
int list_empty(const list_head_t *head);
static  void __list_add(list_head_t *new_lst, list_head_t *prev, list_head_t *next);
void list_add(list_head_t *new_lst, list_head_t *head);
void list_add_tail( list_head_t *new_lst, list_head_t *head);
void list_insert(list_head_t *new_lst, list_head_t *prev, list_head_t *next);
static  void __list_del(list_head_t * prev, list_head_t * next);
void list_del(list_head_t * entry);

static inline int list_is_head(const struct list_head *list, const struct list_head *head)
{
	return list == head;
}


#endif