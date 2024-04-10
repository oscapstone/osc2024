#include "task.h"

task_t* task_head;

void init_task_queue(){
    task_head = (task_t*)simple_alloc(sizeof(task_t));
    task_t* task_end = (task_t*)simple_alloc(sizeof(task_t));

    task_head->prio = -1;
    task_head->prev = (void*)0;
    task_head->next = task_end;

    task_end->prio = LOW_PRIO;
    task_end->prev = task_head;
    task_end->next = (void*)0;
}

void add_task(task_callback_t callback, int prio){
    task_t* new_task = (task_t*)simple_alloc(sizeof(task_t));
    
    new_task->prio = prio;
    new_task->callback = callback;

    disable_interrupt();

    // print_str("\nAdding task...");

    task_t* iter = task_head;
    
    while (iter->prio < LOW_PRIO){
        // print_hex(iter->prio);
        if (new_task->prio < iter->prio)
            break;

        iter = iter->next;
    }

    new_task->next = iter;
    new_task->prev = iter->prev;

    iter->prev = new_task;
    new_task->prev->next = new_task;

    iter = task_head;

    // while (iter->prio < LOW_PRIO){
    //     print_hex(iter->prio);
    //     print_str(" ");
    //     iter = iter->next;
    // }

    enable_interrupt();
}

void pop_task(){

    disable_interrupt();
    // print_str("\nPopping task...");
    task_t* exec_task = task_head->next;
    task_head->next = exec_task->next;
    task_head->next->prev = task_head;
    enable_interrupt();

    exec_task->callback();
}