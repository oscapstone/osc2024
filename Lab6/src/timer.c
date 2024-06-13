#include "timer.h"
#include "uart1.h"
#include "memory.h"
#include "string.h"
#include "exception.h"

#define STR(x) #x
#define XSTR(s) STR(s)

struct list_head *timer_list; // first head has nothing, store timer_event_t after it

void timer_list_init()
{
    unsigned long long tmp;
    asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
    tmp |= 1;
    asm volatile("msr cntkctl_el1, %0" ::"r"(tmp));
    timer_list = kmalloc(sizeof(list_head_t));
    INIT_LIST_HEAD(timer_list);
}

void core_timer_enable()
{
    // __asm__ __volatile__(
    //     "mov x1, 1\n\t"
    //     "msr cntp_ctl_el0, x1\n\t" // cntp_ctl_el0[0]: enable EL1 physical timer.
    //     "mov x2, 2\n\t"
    //     "ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t"
    //     "str w2, [x1]\n\t"         //Non-secure physical timer event IRQ enable
    //                                 //Secure physical timer event IRQ disable
    // );
    __asm__ __volatile__(
        "mov x1, 1\n\t"
        "msr cntp_ctl_el0, x1\n\t" // enable
        "mov x2, 2\n\t"
        "ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t"
        "str w2, [x1]\n\t" // unmask timer interrupt
        ::: "x1", "x2");
}

void core_timer_disable()
{
    __asm__ __volatile__(
        "mov x2, 0\n\t"
        "ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t"
        "str w2, [x1]\n\t" // unmask timer interrupt
        ::: "x1", "x2");
}

void timer_event_callback(timer_event_t *timer_event)
{
    ((void (*)(char *))timer_event->callback)(timer_event->args); // call the callback store in event
    list_del_entry((struct list_head *)timer_event);
    kfree(timer_event->args);
    kfree(timer_event);

    if (!list_empty(timer_list))
    {
        set_timer_interrupt(((timer_event_t *)timer_list->next)->interrupt_time);
    }
    else
    {
        // core_timer_disable();
        set_core_timer_interrupt(10000); // disable timer interrupt (set a very big value)
    }
}

void core_timer_handler()
{
    lock();
    if (list_empty(timer_list))
    {
        // core_timer_disable();
        set_core_timer_interrupt(10000); // disable timer interrupt (set a very big value)
        unlock();
        return;
    }
    timer_event_callback((timer_event_t *)timer_list->next); // do callback and set new interrupt
    unlock();
}

void add_timer(void *callback, unsigned long long timeout, char *args, int tick)
{
    timer_event_t *new_timer = kmalloc(sizeof(timer_event_t));

    new_timer->args = kmalloc(strlen(args) + 1);
    strcpy(new_timer->args, args);

    if (tick == 0)
    {
        new_timer->interrupt_time = get_time_add_sec(timeout);
    }
    else
    {
        new_timer->interrupt_time = get_time_add_sec(0) + timeout;
    }

    new_timer->callback = callback;

    // add new timer to list
    struct list_head *curr;
    lock();
    list_for_each(curr, timer_list)
    {
        // add before the first bigger than new timer
        if (((timer_event_t *)curr)->interrupt_time > new_timer->interrupt_time)
        {
            list_add(&new_timer->listhead, curr->prev);
            break;
        }
    }
    // if new timer should be at the first
    if (list_is_head(curr, timer_list))
    {
        list_add_tail(&new_timer->listhead, timer_list);
    }
    // set interrupt
    set_timer_interrupt(((timer_event_t *)timer_list->next)->interrupt_time);
    unlock();
}

// get current time and add some second
unsigned long long get_time_add_sec(unsigned long long second)
{
    unsigned long long cntpct_el0 = 0;
    __asm__ __volatile__("mrs %0, cntpct_el0\n\t"
                         : "=r"(cntpct_el0)); //tick now

    unsigned long long cntfrq_el0 = 0;
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t"
                         : "=r"(cntfrq_el0)); //tick frequency

    return (cntpct_el0 + cntfrq_el0 * second);
}

// set timer interrupt time to [expired_time] seconds after now (relatively)
void set_core_timer_interrupt(unsigned long long expired_time)
{
    __asm__ __volatile__(
        "mrs x1, cntfrq_el0\n\t"    // cntfrq_el0 -> frequency of the timer
        "mul x1, x1, %0\n\t"        // cntpct_el0 = cntfrq_el0 * seconds: relative timer to cntfrq_el0
        "msr cntp_tval_el0, x1\n\t" // Set expired time to cntp_tval_el0, which stores time value of EL1 physical timer.
        :: "r"(expired_time):"x1");
}

// directly set timer interrupt by seconds
void set_timer_interrupt(unsigned long long sec)
{
    __asm__ __volatile__(
        "msr cntp_cval_el0, %0\n\t" // cntp_cval_el0 -> compare value
        :: "r"(sec));
}

// get timer pending queue size
int timer_list_get_size()
{
    int r = 0;
    struct list_head *curr;
    list_for_each(curr, timer_list)
    {
        r++;
    }
    return r;
}