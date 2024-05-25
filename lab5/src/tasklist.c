#include "tasklist.h"
#include "alloc.h"
#include "exception.h"
#include "mini_uart.h"

task_t *task_head = 0;
int cur_priority = 100;


void create_task(task_callback_t callback, unsigned long long priority) {
    el1_interrupt_disable();
    task_t* task = (task_t*)kmalloc(sizeof(task_t));
    if(!task) return;

    task->callback = callback;
    task->priority = priority;

    enqueue_task(task);
    el1_interrupt_enable();
}

void enqueue_task(task_t *task) {
    el1_interrupt_disable();
    // uart_send_string("enqueue task\n");
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
    el1_interrupt_enable();
}

void execute_tasks() {
    // el1_interrupt_disable();
    while(task_head) {
        el1_interrupt_disable();
        task_t* cur = task_head;
        cur -> callback();
        task_head = cur->next;
        if(task_head) {
            task_head -> prev = 0;
        }
        kfree(cur);
        el1_interrupt_enable();
    }
    // el1_interrupt_enable();
}

void execute_tasks_preemptive() {
    el1_interrupt_enable();
    while(task_head) {
        el1_interrupt_disable();
        task_t* new_task = task_head;

        if(cur_priority <= new_task->priority) {
            el1_interrupt_enable();
            break;
        }
        
        task_head = task_head->next;
        if(task_head) {
            task_head -> prev = 0;
        }
        int prev_priority = cur_priority;
        cur_priority = new_task->priority;

        el1_interrupt_enable();
        // running in preemptive mode

        new_task -> callback();

        el1_interrupt_disable();
        cur_priority = prev_priority;
        kfree(new_task);

        el1_interrupt_enable();

        // free the task: TDB 
    }
}