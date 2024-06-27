#include "irqtask.h"
#include "exception.h"
#include "memory.h"
#include "uart1.h"

int curr_task_priority = 9999;
struct list_head *task_list;

void irqtask_init_list()
{
    task_list = kmalloc(sizeof(list_head_t));
    INIT_LIST_HEAD(task_list);
}

void irqtask_add(void *task_function,unsigned long long priority){

    irqtask_t *the_task = kmalloc(sizeof(irqtask_t)); //need to kfree by task runner
    the_task->priority = priority;
    the_task->task_function = task_function;

    lock();
    struct list_head *curr;
    list_for_each(curr, task_list)
    {
        if (((irqtask_t *)curr)->priority > the_task->priority)
        {
            list_add(&the_task->listhead, curr->prev);
            break;
        }
    }
    if (list_is_head(curr, task_list)) list_add_tail(&(the_task->listhead), task_list); // for the time is the biggest
    unlock();
}

void irqtask_run_preemptive(){
    while (1)
    {
        lock();
        if (list_empty(task_list))
        {
            unlock();
            break;
        }

        irqtask_t *the_task = (irqtask_t *)task_list->next;
        if (curr_task_priority <= the_task->priority)
        {
            unlock();
            break;
        }
        list_del_entry((struct list_head *)the_task);
        int prev_task_priority = curr_task_priority;
        curr_task_priority = the_task->priority;

        unlock();
        irqtask_run(the_task);
        curr_task_priority = prev_task_priority;
        kfree(the_task);
    }
}

void irqtask_run(irqtask_t *the_task)
{
    ((void (*)())the_task->task_function)();
}
