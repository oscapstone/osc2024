#pragma once

#include <stddef.h>

typedef struct _LINK_LIST {
    struct _LINK_LIST *prev, *next;
}LINK_LIST;

#define LLIST_CONTAINER_OF(ptr, type, member) ({ \
    void *__mptr = (void *)(ptr);          \
    ((type *)(__mptr - offsetof(type, member))); })

#define LLIST_ENTRY(ptr, type, member) \
    LLIST_CONTAINER_OF(ptr, type, member)

#define LLIST_FOR_EACH(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define LLIST_FOR_EACH_ENTRY(entry, head, member)                       \
    for (entry = LLIST_ENTRY((head)->next, __typeof__(*entry), member); \
         &entry->member != (head);                                     \
         entry = LLIST_ENTRY(entry->member.next, __typeof__(*entry), member))




void link_list_init(LINK_LIST* list);
void link_list_push_front(LINK_LIST* list, LINK_LIST* v);
void link_list_push_back(LINK_LIST* list, LINK_LIST* v);
