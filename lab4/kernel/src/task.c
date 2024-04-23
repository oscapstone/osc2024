#include "uart.h"
#include "exception.h"
#include "task.h"

#define MAX_TASK_HEAP_SIZE 256

task_heap *task_hp = 0;
int cur_priority = 100;

void task_heap_init()
{
    task_hp = create_task_heap(MAX_TASK_HEAP_SIZE);
}

void do_task()
{
    while (1)
    {
        disable_interrupt();
        int pre_priority = cur_priority; // store the old task's priority
        if(task_hp->size == 0) // heap is empty
        {
            enable_interrupt();
            return;
        }

        task t = task_hp->arr[0];
        if(t.priority >= cur_priority) // new interrupt task's priority dones't be higher than old task's
        {
            enable_interrupt();
            return;
        }
        else
        {
            // new task preempt
            task_heap_extractMin(task_hp);
            cur_priority = t.priority;
        }
        enable_interrupt();
        
        t.callback();

        disable_interrupt();
        cur_priority = pre_priority; // restore the old task's priorty
        enable_interrupt();
    }
}