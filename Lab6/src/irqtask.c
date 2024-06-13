#include "irqtask.h"
#include "exception.h"
#include "memory.h"
#include "uart1.h"

int cur_priority = 9999;
struct list_head *task_list;

void task_list_init()
{
    task_list = kmalloc(sizeof(list_head_t));
    INIT_LIST_HEAD(task_list);
}

// add new task to task list
void add_task_list(void *callback, unsigned long long priority)
{
    irqtask_t *cur_task = kmalloc(sizeof(irqtask_t));

    cur_task->priority = priority;
    cur_task->callback = callback;

    lock();
    struct list_head *ptr;
    list_for_each(ptr, task_list)
    {
        if (((irqtask_t *)ptr)->priority > cur_task->priority)
        {
            list_add(&cur_task->listhead, ptr->prev);
            break;
        }
    }

    if (list_is_head(ptr, task_list))
    {
        list_add_tail(&(cur_task->listhead), task_list);
    }

    unlock();
}

// decide preemtion or not
void preemption()
{
    //lock();
    while (1)
    {
        lock();
        if(list_empty(task_list)){
            unlock();
            break;
        }
        
        irqtask_t *first_task = (irqtask_t *)task_list->next;

        // process executing now has highest priority
        if (cur_priority <= first_task->priority)
        {
            unlock();
            break;
        }

        // executing first task in the task bc its priority is higher
        list_del_entry((struct list_head *)first_task);
        int prev_priority = cur_priority;
        cur_priority = first_task->priority;

        unlock();
        run_task(first_task);
        cur_priority = prev_priority;
        kfree(first_task);
    }
}

// run task callback
void run_task(irqtask_t *task)
{
    ((void (*)())task->callback)();
}
