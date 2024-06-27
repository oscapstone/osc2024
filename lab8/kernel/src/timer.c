#include "timer.h"
#include "uart1.h"
#include "memory.h"
#include "string.h"
#include "exception.h"

#define STR(x) #x
#define XSTR(s) STR(s)

struct list_head *timer_event_list;

void timer_list_init()
{
    unsigned long long tmp;
    asm volatile("mrs %0, cntkctl_el1": "=r"(tmp));
    tmp |= 1;
    asm volatile("msr cntkctl_el1, %0":: "r"(tmp));

    timer_event_list = kmalloc(sizeof(list_head_t));
    INIT_LIST_HEAD(timer_event_list);
}

void core_timer_enable()
{
    __asm__ __volatile__(
        "mov x1, 1\n\t"
        "msr cntp_ctl_el0, x1\n\t" // enable

        "mov x2, 2\n\t"
        "ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t"
        "str w2, [x1]\n\t" // unmask timer interrupt
    :::"x1","x2");
}

void core_timer_disable()
{
    __asm__ __volatile__(
        "mov x2, 0\n\t"
        "ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t"
        "str w2, [x1]\n\t" // unmask timer interrupt
    :::"x1","x2");
}

void timer_event_callback(timer_event_t *timer_event)
{
    ((void (*)(char *))timer_event->callback)(timer_event->args); // call the callback store in event
    list_del_entry((struct list_head *)timer_event);              // delete the event
    kfree(timer_event->args); // kfree the arg space
    kfree(timer_event);

    //set interrupt to next time_event if existing
    if (!list_empty(timer_event_list))
    {
        set_core_timer_interrupt_by_tick(((timer_event_t *)timer_event_list->next)->interrupt_time);
    }
    else
    {
        set_core_timer_interrupt(10000); // disable timer interrupt (set a very big value)
    }
}

void core_timer_handler()
{
    lock();
    //disable_interrupt();
    if (list_empty(timer_event_list))
    {
        set_core_timer_interrupt(10000); // disable timer interrupt (set a very big value)
        unlock();
        return;
    }

    timer_event_callback((timer_event_t *)timer_event_list->next); // do callback and set new interrupt
    unlock();
}

// give a string argument to callback   timeout after seconds
void add_timer(void *callback, unsigned long long timeout, char *args, int bytick)
{
    timer_event_t *the_timer_event = kmalloc(sizeof(timer_event_t)); //need to kfree by event handler

    // store argument string into timer_event
    the_timer_event->args = kmalloc(strlen(args) + 1);
    strcpy(the_timer_event->args, args);

    if(bytick == 0)
    {
        the_timer_event->interrupt_time = get_tick_plus_s(timeout); // store interrupt time into timer_event
    }else
    {
        the_timer_event->interrupt_time = get_tick_plus_s(0) + timeout;
    }

    the_timer_event->callback = callback;

    // add the timer_event into timer_event_list (sorted)
    struct list_head *curr;

    lock();
    list_for_each(curr, timer_event_list)
    {
        if (((timer_event_t *)curr)->interrupt_time > the_timer_event->interrupt_time)
        {
            list_add(&the_timer_event->listhead, curr->prev); // add this timer at the place just before the bigger one (sorted)
            break;
        }
    }

    if (list_is_head(curr, timer_event_list))
    {
        list_add_tail(&the_timer_event->listhead, timer_event_list); // for the time is the biggest
    }


    // set interrupt to first event
    set_core_timer_interrupt_by_tick(((timer_event_t *)timer_event_list->next)->interrupt_time);
    unlock();
}


// get cpu tick add some second
unsigned long long get_tick_plus_s(unsigned long long second)
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
        "mrs x1, cntfrq_el0\n\t" //cntfrq_el0 -> relative time
        "mul x1, x1, %0\n\t"
        "msr cntp_tval_el0, x1\n\t" // set expired time
        :: "r"(expired_time):"x1");
}

// directly set timer interrupt time to a cpu tick  (directly)
void set_core_timer_interrupt_by_tick(unsigned long long tick)
{
    __asm__ __volatile__(
        "msr cntp_cval_el0, %0\n\t" //cntp_cval_el0 -> absolute time
        :: "r"(tick));
}

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
