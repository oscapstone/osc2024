#include "tasklist.h"
#include "allocator.h"
#include "uart.h"
#include "printf.h"
#include "interrupt.h"

task_t *task_head = NULL;
uint64_t task_count = 0;

void enqueue_task(task_t *new_task) {
    if (!task_head) {
		new_task->next = task_head;
        new_task->prev = NULL;
        task_head = new_task;
    }
    else{

        if (new_task->priority <= task_head->priority){         //new_task的優先級比開頭的任務還大
            // new_task->callback();
            new_task->next = task_head;
            new_task->prev = NULL;
            task_head->prev = new_task;
            task_head = new_task;
        }
        else {
            // Find the correct position in the list
            task_t *current = task_head;
            while (current->next && current->next->priority <= new_task->priority) {
                current = current->next;
            }
            // Insert the new task
            new_task->next = current->next;
            new_task->prev = current;
            if (current->next) {
                current->next->prev = new_task;
            }
            current->next = new_task;
        }

    } 
}


void create_task(task_callback callback, uint64_t priority) {
    disable_interrupt();
    task_count++;
	task_t* task = simple_malloc(sizeof(task_t));
	if(!task) {
		return;
	}

	task->callback = callback;
	task->priority = priority;
    task->task_num = task_count;

	enqueue_task(task);    
}


void execute_tasks(uint64_t elr,uint64_t spsr) {
    
    while (task_head) { 
        int exetasknum = task_head->task_num;
        // printf("execute \n");
        task_head->callback();
        // printf("execute completed\n");
        if(exetasknum != task_head->task_num){
            continue;
        }
        else if (task_head->next) {
            // printf("task_head->next\n");
            task_head = task_head->next;
            task_head->prev = NULL;
            
        }
        else{
            // printf("ELSE\n");
            asm volatile("msr elr_el1,%0"::"r"(elr));
            asm volatile("msr spsr_el1,%0"::"r"(spsr));
            // asm volatile("msr elr_el1,%0"::"r"(task_head->elr));
            // asm volatile("msr spsr_el1,%0"::"r"(task_head->spsr));
            task_head = NULL;
            task_count = 0;
        }
    }
}