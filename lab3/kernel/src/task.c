#include "task.h"
#include "alloc.h"
#include "mini_uart.h"
#include "utils.h"

#include <stddef.h>

task *task_head;

void push_task(task *t) {
    asm volatile("msr DAIFSet, 0xf");
    
	if (task_head == NULL) {
		task_head = simple_malloc(sizeof(task));
		task_head -> next = NULL;
	}
	
	task* cur = task_head;
	while (cur -> next != NULL && cur -> next -> p < t -> p) {
		cur = cur -> next;
	}

	t -> next = cur -> next;
	cur -> next = t;

	asm volatile("msr DAIFClr, 0xf");
}

void create_task(task_callback callback, int priority) {
	task* t = simple_malloc(sizeof(task));

	t -> callback = callback;
	t -> p = priority;
		
	push_task(t);
}

void execute_tasks() {
    while (task_head -> next != NULL) { 
        task_head -> next -> callback();
		task_head = task_head -> next;
    }
	irq (1);
}
