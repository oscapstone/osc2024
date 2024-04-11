#include "task.h"
#include "malloc.h"
#include "timer.h"
#include "uart.h"

int cur_priority = 10;
struct list_head task_list;
LIST_HEAD(task_list);

void add_task(int priority, task_callback_t callback)
{
    task_node *entry = (task_node *)simple_malloc(sizeof(task_node));
    entry->priority = priority;
    entry->callback = callback;
    INIT_LIST_HEAD(&entry->list);

    asm volatile("msr DAIFSet, 0xf");
    if (list_empty(&task_list)) {
        list_add_tail(&entry->list, &task_list);
    }
    else {
        task_node *node;
        struct list_head *p;
        int flag = 0;
        list_for_each(p, &task_list)
        {
            node = list_entry(p, task_node, list);

            if (entry->priority < node->priority) {
                list_add(&entry->list, p->prev);
                flag = 1;
                break;
            }
        }

        if (!flag) {
            list_add_tail(&entry->list, &task_list);
        }
    }
    asm volatile("msr DAIFClr, 0xf");
    uart_puts("pop_task\n");
}

void pop_task()
{
    uart_async_puts("pop_task\n");
    while (!list_empty(&task_list)) {
        task_node *first = list_entry(&task_list, task_node, list);
        if (first->priority > cur_priority)
            return;

        asm volatile("msr DAIFSet, 0xf");
        list_del(&first->list);
        int tmp_priority = cur_priority;
        cur_priority = first->priority;
        uart_dec(cur_priority);
        asm volatile("msr DAIFClr, 0xf");

        first->callback();

        asm volatile("msr DAIFSet, 0xf");
        cur_priority = tmp_priority;
        asm volatile("msr DAIFClr, 0xf");
    }
}