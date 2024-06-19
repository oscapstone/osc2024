#include "tasklist.h"
#include "alloc.h"
#include "exception.h"
#include "mini_uart.h"

task_t *task_head = 0;
int cur_priority = 100;

void create_task(task_callback_t callback, unsigned long long priority) {
    disable_interrupt();
    task_t* task = simple_malloc(sizeof(task_t));
    if(!task) return;
    // uart_send_string_int2hex(task);
    // uart_send_string("\r\n");
    task->callback = callback;
    task->priority = priority;

    enqueue_task(task);
    enable_interrupt();
}

void enqueue_task(task_t *task) {
    disable_interrupt();
    if(!task_head || (task->priority < task_head->priority)) {
        task -> next = task_head;
        task -> prev = 0;
        if(task_head)
            task_head->prev = task;
        task_head = task;
    } else {
        task_t *current = task_head;
        while(current->next && current->next->priority <= task->priority) {
            current = current->next;
        }
        task->next = current->next;
        task->prev = current;
        if (current -> next){
            current -> next -> prev = task;
        }
        current -> next = task;
    }
    enable_interrupt();
    return;
}

// void execute_tasks() {
//     // disable_interrupt();
//     while(task_head) {
//         disable_interrupt();
//         task_head -> callback();
//         task_head = task_head->next;
//         if(task_head) {
//             task_head -> prev = 0;
//         }
//         enable_interrupt();
//     }
//     // enable_interrupt();
// }

void execute_tasks_preemptive() {
    enable_interrupt();
    while(task_head) {
        disable_interrupt();
        task_t* new_task = task_head;

        if(cur_priority <= new_task->priority) {
            enable_interrupt();
            break;
        }
        
        task_head = task_head->next;
        if(task_head) {
            task_head -> prev = 0;
        }
        int prev_priority = cur_priority;
        cur_priority = new_task->priority;

        enable_interrupt();
        // running in preemptive mode

        new_task -> callback();

        disable_interrupt();

        cur_priority = prev_priority;

        enable_interrupt();

        // free the task: TDB 
    }
}