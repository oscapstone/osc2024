#include "tasklist.h"
#include "heap.h"
#include "uart.h"

task_t *task_head = NULL;

void enqueue_task(task_t *new_task)
{
    // uart_puts("enqueue a task, Name: ");
    // uart_puts(new_task->name);
    // uart_puts("\n");
    // Disable interrupts to protect the critical section
    disable_irq();
    // Special case: the list is empty or the new task has higher priority
    if (!task_head || new_task->priority < task_head->priority)
    {
        new_task->next = task_head;
        new_task->prev = NULL;
        if (task_head)
        {
            task_head->prev = new_task;
        }
        task_head = new_task;
    }
    else
    {
        // Find the correct position in the list
        task_t *current = task_head;
        while (current->next && current->next->priority <= new_task->priority)
        {
            current = current->next;
        }

        // Insert the new task
        new_task->next = current->next;
        new_task->prev = current;
        if (current->next)
        {
            current->next->prev = new_task;
        }
        current->next = new_task;
    }

    // Enable interrupts
    enable_irq();
    // task_t *temp = task_head;
    // uart_puts("task in list:\n");
    // while (temp)
    // {
    //     //uart_puts(current->name);
    //     uart_puts(temp->name);
    //     uart_puts("\n");
    //     temp = temp->next;
    // }
}

void create_task(task_callback callback, uint64_t priority, void *data, char *name)
{
    // uart_puts("task created, Name: ");
    // uart_puts(name);
    // uart_puts("\n");
    task_t *task = kmalloc(sizeof(task_t));
    if (!task)
    {
        return;
    }

    task->callback = callback;
    task->priority = priority;
    task->userdata = data;
    task->name = name;

    enqueue_task(task);
}

void execute_tasks()
{
    while (task_head)
    {
        //disable_irq(); // Disable interrupts
        // uart_puts("executing, Name: ");
        // uart_puts(task_head->name);
        // uart_puts("\n");
        task_head->callback(task_head->userdata);
        task_head = task_head->next;
        if (task_head)
        {
            task_head->prev = NULL;
        }
        // simple_free(task);
        enable_irq(); // Enable interrupts
    }

    
}
