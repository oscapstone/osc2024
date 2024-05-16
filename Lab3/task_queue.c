#include "shell.h"
#include "uart.h"
#include "timer.h"

#define MAX_TASKS 1024 //if boom will boom

char in_buffer[1024];
char exec_buffer[1024];
int in_idx;
int exec_idx;

typedef void (*task_func_t)(void);

typedef struct {
    task_func_t func;
    //void* data;
    int priority; // Optional for prioritized execution
    int state; //0: wait, 1: running, -1 end
} task_t;

typedef struct {
    task_t tasks[MAX_TASKS];
    int min_priority;
    int task_count;
    int running;
} task_queue_t;

task_queue_t task_queue;
int init;

void init_task_queue(){
    task_queue.running = -1;
}

void create_task(task_func_t callback, unsigned int priority){
    if(init == 0){
        init_task_queue();
        init = 1;
    }
    //uart_puts("task created!\n");
    int i;
    for(i=0; i<MAX_TASKS;i++){
        if(task_queue.tasks[i].priority == 0){
            task_queue.task_count++;
            task_queue.tasks[i].priority = priority;
            task_queue.tasks[i].func = callback;
            task_queue.tasks[i].state = 0;
            if(priority < task_queue.min_priority || task_queue.min_priority == 0)
                task_queue.min_priority = priority;
            // uart_puts("current min priority: ");
            // uart_int(task_queue.min_priority);
            break;
        }
    }
}

// asm volatile("msr DAIFSet, 0xf");
// asm volatile("msr DAIFClr, 0xf");
void execute_task(){
    while(task_queue.task_count != 0){
        int next_min = 999;
        for(int i=0; i<MAX_TASKS;i++){
            if(task_queue.running != -1 && task_queue.tasks[i].priority >= task_queue.tasks[task_queue.running].priority){ 
                // only preempt if priority lower than current running
                continue;
            }
            if(task_queue.tasks[i].priority == task_queue.min_priority && task_queue.tasks[i].state == 0){
                // asm volatile("msr DAIFSet, 0xf");
                int saved_run = task_queue.running; //current running
                task_queue.task_count--;
                task_queue.tasks[i].state = 1;
                task_queue.running = i;
                task_queue.tasks[i].func();
                task_queue.tasks[i].priority = 0;
                task_queue.tasks[i].state = -1;
                task_queue.running = saved_run; // restore to last running task id
                // asm volatile("msr DAIFClr, 0xf");
            }
            else if(task_queue.tasks[i].priority != 0){
                if(task_queue.tasks[i].priority < next_min)
                    next_min = task_queue.tasks[i].priority;
            }
        }
        if(next_min!=999 && (task_queue.running == -1 || next_min < task_queue.tasks[task_queue.running].priority))
            task_queue.min_priority = next_min;
        // else
        //     task_queue.min_priority = 0;
    }
    //uart_puts("By Execute\n\r");
}

void show_buffer(){
    uart_puts("\n");
    uart_puts("Queued:\n");
    uart_puts(in_buffer);
    uart_puts("\n");
    uart_puts("Exec: \n");
    uart_puts(exec_buffer);
    uart_puts("\n");
}
