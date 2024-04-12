#include "timer.h"
#include "malloc.h"
#include "string.h"
#include "uart.h"

struct list_head timer_list;
LIST_HEAD(timer_list);

void core_timer_enable() { asm volatile("msr cntp_ctl_el0, %0" : : "r"(1)); }

void core_timer_disable() { asm volatile("msr cntp_ctl_el0, %0" : : "r"(0)); }

void core_timer_interrupt_enable()
{
    asm volatile("mov x2, 2\n\t");
    asm volatile("ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t");
    asm volatile("str w2, [x1]\n\t");
}

void core_timer_interrupt_disable()
{
    asm volatile("mov x2, 0\n\t");
    asm volatile("ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t");
    asm volatile("str w2, [x1]\n\t");
}

void set_core_timer_interrupt()
{
    timer_node *entry = list_first_entry(&timer_list, timer_node, list);
    // uart_dec(entry->expired_time);
    asm volatile("msr cntp_cval_el0, %0" ::"r"(entry->expired_time)); // set expired time
}

void set_core_timer_interrupt_permanent()
{
    unsigned long long sec = 0xFFFFFFFFFFFFFFFF;
    // unsigned long long sec = 0xFFF;
    asm volatile("msr cntp_cval_el0, %0" ::"r"(sec)); // set expired time
}

void add_timer(timer_callback_t callback, void *arg, unsigned long long expired_time)
{
    timer_node *entry = (timer_node *)simple_malloc(sizeof(timer_node));

    entry->args = (char *)simple_malloc(20);
    strcpy(entry->args, (char *)arg);
    entry->callback = callback;
    entry->expired_time = expired_time;
    INIT_LIST_HEAD(&entry->list);

    asm volatile("msr DAIFSet, 0xf");
    if (list_empty(&timer_list)) {
        list_add_tail(&entry->list, &timer_list);
    }
    else {
        add_node(&timer_list, entry);
    }
    set_core_timer_interrupt();
    asm volatile("msr DAIFClr, 0xf");
    core_timer_interrupt_enable();
}

void add_node(struct list_head *head, timer_node *entry)
{
    struct list_head *p;
    list_for_each(p, head)
    {
        timer_node *node = list_entry(p, timer_node, list);
        if (entry->expired_time < node->expired_time) {
            list_add(&entry->list, p->prev);
            return;
        }
    }
    list_add_tail(&entry->list, head);
}

void pop_timer()
{
    timer_node *first = list_first_entry(&timer_list, timer_node, list);
    asm volatile("msr DAIFSet, 0xf");
    list_del(&first->list);
    asm volatile("msr DAIFClr, 0xf");

    first->callback(first->args);

    asm volatile("msr DAIFSet, 0xf");
    if (list_empty(&timer_list)) {
        set_core_timer_interrupt_permanent();
    }
    else {
        set_core_timer_interrupt();
    }
    asm volatile("msr DAIFClr, 0xf");
}

void core_timer_handler()
{
    if (list_empty(&timer_list)) {
        set_core_timer_interrupt_permanent();
        return;
    }

    pop_timer();
}