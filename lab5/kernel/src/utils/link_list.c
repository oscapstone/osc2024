
#include "link_list.h"


void link_list_init(LINK_LIST* list) {
    list->next = list;
    list->prev = list;
}

void link_list_push_front(LINK_LIST* list, LINK_LIST* v) {
    v->next = list->next;
    v->prev = list;
    list->next->prev = v;
    list->next = v;
}

void link_list_push_back(LINK_LIST* list, LINK_LIST* v) {
    v->next = list;
    v->prev = list->prev;
    list->prev->next = v;
    list->prev = v;
}

