#include "task.h"

list_head_t task_list;
int cur_priority = 10;

void add_task(task_callback_t callback, int priority){
    // init task

    task_t *t = (task_t*)simple_malloc(sizeof(task_t));
    // uart_puts("a\n");
    t->callback = callback;
    // uart_puts("b\n");
    t->priority = priority;
    t->listhead.next = &(t->listhead);
    t->listhead.prev = &(t->listhead);

    disable_interrupt();
    // insert based on priority
    if(list_empty(&task_list)) {
        //uart_puts("f\n");
        list_add_tail(&t->listhead, &task_list);
    }
    else {
        task_t *now;
        list_head_t *listptr;
        int inst = 0;
        list_for_each(listptr, &task_list) {
            now = list_entry(listptr, task_t, listhead);
            
            if(priority < now->priority) {
                list_insert(&t->listhead, listptr->prev, listptr);
                inst = 1;
                break;
            }
        }
        if(!inst) {
            list_add_tail(&t->listhead, &task_list);
        }
    }

    enable_interrupt();
}

void pop_task() {
    while (!list_empty(&task_list)) {
        task_t *first = (task_t *)list_entry(task_list.next, task_t, listhead);
        if(first->priority > cur_priority) {
            return;
        }
        disable_interrupt();
        list_del(&first->listhead);
        int tmp_priority = cur_priority;
        cur_priority = first->priority;
        enable_interrupt();

        first->callback();

        disable_interrupt();
        cur_priority = tmp_priority;
        enable_interrupt();
    }
}

