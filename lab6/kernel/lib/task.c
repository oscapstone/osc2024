#include "task.h"
#include "irq.h"
#include "string.h"
#include "malloc.h"
#include "uart.h"
#include "timer.h"

list_head_t task_list;
int cur_priority = 10;

void task_list_init() {
    INIT_LIST_HEAD(&task_list);
}
void add_task(task_callback_t callback, int priority){
    // init task
    //uart_puts("t1\n");
    task_t *t = malloc(sizeof(task_t));
    //uart_puts("t2\n");
    t->priority = priority;
    
    //uart_puts("t3\n");
    t->callback = callback;
    //uart_puts("t4\n");

    INIT_LIST_HEAD(&t->listhead);
    
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
        disable_interrupt();
        task_t *first = (task_t *)list_entry(task_list.next, task_t, listhead);
        if(first->priority > cur_priority) {
            enable_interrupt();
            return;
        }
       
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

