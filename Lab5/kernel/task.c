#include "task.h"

task_t* task_head;
int cur_prio = LOW_PRIO;

extern thread_t* cur_thread;

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
    task_t* new_task = (task_t*)malloc(sizeof(task_t));
    
    new_task->prio = prio;
    new_task->callback = callback;

    disable_interrupt();

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

    
    // if (cur_thread->id == 2){
    //     print_str("\nADD: ");
    //     while (iter->prio < LOW_PRIO){
    //         print_hex(iter->prio);
    //         print_str(" ");
    //         iter = iter->next;
    //     }
    // }

    enable_interrupt();
}

void pop_task(){

    disable_interrupt();

    // if (cur_thread->id == 2){
    //     print_str("\nPopping task...");
    // }
    // print_str("\nPopping task...");
    task_t* exec_task = task_head->next;
    
    task_t* iter = task_head;
    if (exec_task->prio == LOW_PRIO){
        enable_interrupt();
        return;
    }

    // print_newline();
    // print_hex(exec_task->prio);

    // print_str("\ncur_prio: ");
    // print_hex(cur_prio);

    if (exec_task->prio > cur_prio)
        return;

    // if (cur_thread->id == 2){
    //     print_str("\nPOP: ");
    //     while (iter->prio < LOW_PRIO){
    //         print_hex(iter->prio);
    //         print_str(" ");
    //         iter = iter->next;
    //     }
    // }

    int orin_prio = cur_prio;

    task_head->next = exec_task->next;
    task_head->next->prev = task_head;
    cur_prio = exec_task->prio;
    enable_interrupt();

    // print_hex(cur_prio);
    exec_task->callback();

    disable_interrupt();
    free(exec_task);
    cur_prio = orin_prio;
    enable_interrupt();
}