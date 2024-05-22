#include "list.h"

//list_head_t *list = (list_head_t*)smalloc(sizeof(list_head_t));


void INIT_LIST_HEAD(list_head_t *head)
{
        head->next = head;
        head->prev = head;
}

int list_empty(const list_head_t *head)
{
        return head->next == head;
}

static  void __list_add(list_head_t *new_lst, list_head_t *prev, list_head_t *next)
{
        
        next->prev = new_lst;
        new_lst->next = next;
        new_lst->prev = prev;
        prev->next = new_lst;
}

void list_insert(list_head_t *new_lst, list_head_t *prev, list_head_t *next)
{
        next->prev = new_lst;
        new_lst->next = next;
        new_lst->prev = prev;
        prev->next = new_lst;
}

void list_add(list_head_t *new_lst, list_head_t *head)
{
        __list_add(new_lst, head, head->next);
}

void list_add_tail( list_head_t *new_lst, list_head_t *head)
{
        __list_add(new_lst, head->prev, head);
}

static  void __list_del(list_head_t * prev, list_head_t * next  )
{
        next->prev = prev;
        prev->next = next;
}

void list_del(list_head_t * entry)
{       
        __list_del(entry->prev, entry->next);

}


