#include "header/task.h"
#include "header/list.h"
#include "header/malloc.h"
#include "header/uart.h"

int curr_task_priority = 100;
task *task_list = 0;

void irqtask_list_init(){
    task_list = 0;
}
void add_task(void *task_function, int priority){
    asm volatile("msr DAIFSet, 0xf");
    // make new node
    task *cur = simple_malloc(sizeof(task));
    cur -> task_function = task_function;
    cur -> priority = priority;

    // enqueue
    asm volatile("msr DAIFSet, 0xf");
    // insert if task list is empty
    if(task_list == 0 || cur->priority < task_list -> priority){
        cur -> next = task_list;
        cur -> prev = 0;
        if(task_list == 0){
            task_list -> prev = cur;
        }
        task_list = cur;
    }
    // find place to insert
    else{
        task *node = task_list;
        while (node -> next != 0 && node -> next -> priority <= cur ->priority) {
            node = node -> next;
        }
        cur -> prev = node;
        cur -> next = node -> next;
        if(node -> next != 0){
            node -> next -> prev = cur;
        }
        node -> next = cur;
    }
    asm volatile("msr DAIFClr, 0xf");
    asm volatile("msr DAIFClr, 0xf");
}

void run_task(){
    asm volatile("msr DAIFClr, 0xf");
    while (task_list) {
        asm volatile("msr DAIFSet, 0xf");
        task *tar = task_list;
        if(curr_task_priority <= tar->priority){
            asm volatile("msr DAIFClr, 0xf");
            break;
        }
        // moving
        task_list = task_list->next;
        // removing prev task as it will be exec
        if(task_list)
            task_list -> prev = 0;
        int prev_prio = curr_task_priority;
        curr_task_priority = tar->priority;
        // exec task
        asm volatile("msr DAIFClr, 0xf");
        ((void (*)())tar->task_function)();
        asm volatile("msr DAIFSet, 0xf");
        
        curr_task_priority = prev_prio;
        asm volatile("msr DAIFClr, 0xf");
    }
    
}

void high_prio(){
    uart_async_puts("\r\nhigh priority testing\r\n");
    for(int i = 0;i<10000;i++);
    uart_async_puts("high priority end\r\n");
}
void low_prio(){
    uart_async_puts("\r\nlow priority testing\r\n");
    for(int i = 0;i<10000;i++);
    uart_async_puts("low priority end\r\n");
}
void test_preempt(){
    add_task(low_prio, 9);
    add_task(high_prio, 0);
}
