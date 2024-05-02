#include "header/tasklist.h"
#include "header/allocator.h"
#include "header/uart.h"

task_t *task_head = NULL;

void enqueue_task(task_t *new_task) {
    // Disable interrupts to protect the critical section
    asm volatile("msr DAIFSet, 0xf");
    
    
    // 如果task list是空或是新的task優先度最高
    // priority數字越小, 優先度越高
    if (!task_head || new_task->priority < task_head->priority) {
	new_task->next = task_head;
        new_task->prev = NULL;
        
        // 不是空
        if (task_head) {
            task_head->prev = new_task;
        }
        task_head = new_task;
    } else {
        // 判斷哪個task優先度最高
        // ex : 1 -> 2, 3
        task_t *current = task_head;
        while (current->next && current->next->priority <= new_task->priority) {
            current = current->next;
        }
        
        // 1->2->3
        // Insert the new task
        new_task->next = current->next;
        new_task->prev = current;
        
        // 如果後面還有
        // ex : 1 -> 4, 2
        if (current->next) {
            current->next->prev = new_task;
        }
        current->next = new_task;
    }

    // Enable interrupts
    asm volatile("msr DAIFClr, 0xf");
}

void create_task(task_callback callback, uint64_t priority) {

	task_t* task = simple_malloc(sizeof(task_t));
	if(!task) {
		return;
	}
        
        // callback是irq_except_handler_c()
	task->callback = callback;
	task->priority = priority;
		
	enqueue_task(task);
}

void execute_tasks() {
	

    while (task_head) {        
        task_head->callback();
        task_head = task_head->next;
        if (task_head) {
            task_head->prev = NULL;
        }
	asm volatile("msr DAIFSet, 0xf"); // Disable interrupts
        //simple_free(task);
    }

    asm volatile("msr DAIFClr, 0xf"); // Enable interrupts
}

