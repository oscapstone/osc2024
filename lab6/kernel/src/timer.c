#include "timer.h"
#include "stdio.h"
#include "stdint.h"
#include "memory.h"
#include "string.h"
#include "uart1.h"
#include "stddef.h"
#include "callback_adapter.h"

#define STR(x) #x
#define XSTR(s) STR(s)

struct list_head *timer_event_list; // first head has nothing, store timer_event_t after it

void timer_list_init()
{
    timer_event_list = kmalloc(sizeof(timer_event_t));
    INIT_LIST_HEAD(timer_event_list);
    uint64_t tmp;
    asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
    tmp |= 1;
    asm volatile("msr cntkctl_el1, %0" ::"r"(tmp));
}

void core_timer_enable()
{
    //DEBUG("core_timer_enable\r\n");
    __asm__ __volatile__(
        "mov x1, 1\n\t"
        "msr cntp_ctl_el0, x1\n\t" // cntp_ctl_el0[0]: enable, Control register for the EL1 physical timer.
                                   // cntp_tval_el0: Holds the timer value for the EL1 physical timer
        "mov x2, 2\n\t"
        "ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t"
                                               "str w2, [x1]\n\t" // QA7_rev3.4.pdf: Core0 Timer IRQ allows Non-secure physical timer(nCNTPNSIRQ)
    );
}

void core_timer_disable()
{
    //DEBUG("core_timer_disable\r\n");
    __asm__ __volatile__(
        "mov x2, 0\n\t"
        "ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t"
                                               "str w2, [x1]\n\t" // QA7_rev3.4.pdf: Mask all timer interrupt
    );
}

void core_timer_handler()
{
    //DEBUG("core_timer_handler\r\n");
    // if the queue is empty, do nothing, otherwise, do callback and enable interrupt
    if (list_empty(timer_event_list))
    {
        return;
    }
    timer_event_callback((timer_event_t *)timer_event_list->next); // do callback and set new interrupt
    core_timer_enable();
}

void timer_event_callback(timer_event_t *timer_event)
{
    list_del_entry((struct list_head *)timer_event);                     // delete the event in queue
    ((void (*)(char *))timer_event->callback)(timer_event->args_struct); // call the event
    if (timer_event->args_struct != NULL)
        kfree(timer_event->args_struct); // free the event's space
    kfree(timer_event);

    list_head_t *curr;
    // DEBUG_BLOCK({
    //     uart_puts("\n---------------------- list for each ----------------------\r\n");
    //     list_for_each(curr, timer_event_list)
    //     {
    //         uart_puts("timer_event: 0x%x, timer_event->next: 0x%x, timer_event->prev: 0x%x, timer_event->interrupt_time: %d\r\n", curr, curr->next, curr->prev, ((timer_event_t *)curr)->interrupt_time);
    //     }
    //     uart_puts("--------------------------- end ---------------------------\r\n");
    // });

    // set queue linked list to next time event if it exists
    // if the queue is empty, after this interrupt, the timer will be disabled
    if (!list_empty(timer_event_list))
    {
        set_core_timer_interrupt_by_tick(((timer_event_t *)timer_event_list->next)->interrupt_time);
    }
    //DEBUG("timer_event_callback end\r\n");
}

void timer_set2sAlert()
{
    uint64_t cntpct_el0;
    __asm__ __volatile__("mrs %0, cntpct_el0\n\t" : "=r"(cntpct_el0)); // tick auchor
    uint64_t cntfrq_el0;
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t" : "=r"(cntfrq_el0)); // tick frequency
    uart_puts("\r\n[Start Alert]\r\n");

#ifdef QEMU
    for (int i = 0; i < 1000000000; i++) // qemu
#else
    for (int i = 0; i < 10000000; i++) // pi
#endif
    {
        // do nothing
    }

    uart_puts("[Interrupt][el1_irq] %d seconds after booting\n", cntpct_el0 / cntfrq_el0);
    add_timer_by_sec(2, adapter_timer_set2sAlert, NULL);
}

void add_timer_by_sec(uint64_t sec, void *callback, void *args_struct)
{
    INFO("Add timer event: %d\r\n", sec);
    add_timer_by_tick(sec_to_tick(sec), callback, args_struct);
}

void add_timer_by_tick(uint64_t tick, void *callback, void *args_struct)
{
    // DEBUG("add_timer_by_tick: %d\r\n", tick);
    timer_event_t *the_timer_event = kmalloc(sizeof(timer_event_t)); // free by timer_event_callback
    // store all the related information in timer_event
    the_timer_event->args_struct = args_struct;
    the_timer_event->interrupt_time = get_tick_plus_t(tick);
    // DEBUG("the_timer_event->interrupt_time: %d\r\n", the_timer_event->interrupt_time);
    the_timer_event->callback = callback;
    INIT_LIST_HEAD(&the_timer_event->listhead);

    // add the timer_event into timer_event_list (sorted)
    struct list_head *curr;
    list_for_each(curr, timer_event_list)
    {
        if (((timer_event_t *)curr)->interrupt_time > the_timer_event->interrupt_time)
        {
            list_add(&the_timer_event->listhead, curr->prev); // add this timer at the place just before the bigger one (sorted)
            break;
        }
    }
    // if the timer_event is the biggest, run this code block
    if (list_is_head(curr, timer_event_list))
    {
        list_add_tail(&the_timer_event->listhead, timer_event_list);
    }

    // DEBUG_BLOCK({
    //     struct list_head *tmp;
    //     uart_puts("---------------------- list for each ----------------------\r\n");
    //     list_for_each(tmp, timer_event_list)
    //     {
    //         if (((timer_event_t *)tmp)->callback == adapter_timer_set2sAlert)
    //             uart_puts("adapter_timer_set2sAlert: ");
    //         else if (((timer_event_t *)tmp)->callback == adapter_schedule_timer)
    //             uart_puts("adapter_schedule_timer: ");
    //         else
    //             uart_puts("default: ");
    //         uart_puts("timer_event: 0x%x, timer_event->next: 0x%x, timer_event->prev: 0x%x, timer_event->interrupt_time: %d\r\n", tmp, tmp->next, tmp->prev, ((timer_event_t *)tmp)->interrupt_time);
    //     }
    //     uart_puts("--------------------------- end ---------------------------\r\n");
    // });
    // set interrupt to first event
    set_core_timer_interrupt_by_tick(((timer_event_t *)timer_event_list->next)->interrupt_time);
    core_timer_enable();
}

// get cpu tick add some tick
uint64_t get_tick_plus_t(uint64_t tick)
{
    uint64_t cntpct_el0 = 0;
    __asm__ __volatile__("mrs %0, cntpct_el0\n\t" : "=r"(cntpct_el0)); // tick auchor
    //DEBUG("cntpct_el0: %d\r\n", cntpct_el0);
    return (cntpct_el0 + tick);
}

// get cpu tick add some second
uint64_t sec_to_tick(uint64_t second)
{
    uint64_t cntfrq_el0 = 0;
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t" : "=r"(cntfrq_el0)); // tick frequency
    return (cntfrq_el0 * second);
}

// set timer interrupt time to [expired_time] seconds after now (relatively)
void set_core_timer_interrupt(uint64_t expired_time)
{
    __asm__ __volatile__(
        "mrs x1, cntfrq_el0\n\t"    // cntfrq_el0 -> frequency of the timer
        "mul x1, x1, %0\n\t"        // cntpct_el0 = cntfrq_el0 * seconds: relative timer to cntfrq_el0
        "msr cntp_tval_el0, x1\n\t" // Set expired time to cntp_tval_el0, which stores time value of EL1 physical timer.
        : "=r"(expired_time));
}

// directly set timer interrupt time to a cpu tick  (directly)
void set_core_timer_interrupt_by_tick(uint64_t tick)
{
    __asm__ __volatile__(
        "msr cntp_cval_el0, %0\n\t" // cntp_cval_el0 -> absolute timer
        : "=r"(tick));
}

// get timer pending queue size
int timer_list_get_size()
{
    int r = 0;
    struct list_head *curr;
    list_for_each(curr, timer_event_list)
    {
        r++;
    }
    return r;
}
