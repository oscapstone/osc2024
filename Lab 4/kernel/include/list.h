#ifndef _LIST_H_
#define _LIST_H_

typedef struct list_head {
    struct list_head *next, *prev;
} list_head_t;

typedef list_head_t* list_head_ptr_t;


/* 
    struct list_head mylist = {&mylist, &mylist} ;
*/
#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)


/*
    Usage:
        struct list_head myhead; 
        INIT_LIST_HEAD(&myhead);
*/
static inline void 
INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}


/*
    Insert a new entry between two known consecutive entries.
 */
static inline void 
__list_add(struct list_head *new_, struct list_head *prev, struct list_head *next)
{
    next->prev = new_;
    new_->next = next;
    new_->prev = prev;
    prev->next = new_;
}


 /*
    Insert a new entry after the specified head.
 */
static inline void 
list_add(struct list_head *new_, struct list_head *head)
{
    __list_add(new_, head, head->next);
}


/* 
    Insert a new entry before the specified head.
 */
static inline void 
list_add_tail(struct list_head *new_, struct list_head *head)
{
    __list_add(new_, head->prev, head);
}


/*
    Delete a list entry by making the prev/next entries point to each other.
 */
static inline void 
__list_del(struct list_head * prev, struct list_head * next)
{
    next->prev = prev;
    prev->next = next;
}


/*
    list_del - deletes entry from list.
*/
static inline void
list_del_entry(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    entry->next = 0;
    entry->prev = 0;
}


/*
    check if it is pointing to the list head
*/
static inline int
list_is_head(const struct list_head *list, const struct list_head *head)
{
    return list == head;
}


/*
    check if is is the first entry of the list
*/
static inline int
list_is_first(const struct list_head *list, const struct list_head *head)
{
    return list->prev == head;
}


/*
    check if the list is empty
*/
static inline int
list_empty(const struct list_head *head)
{
    return head->next == head;
}


/*
    iterate over a list
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; !list_is_head(pos, (head)); pos = pos->next)



#endif