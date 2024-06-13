#include "timer.h"
#include "mini_uart.h"
#include "utility.h"
#include "memory.h"

// extern void core_timer_enable1(void);
// extern void core_timer_handler1(void);

struct list_head *timer_event_list; // first head has nothing, store timer_event_t after it

void timer_list_init()
{
    timer_event_list = kmalloc(sizeof(timer_event_t));
    INIT_LIST_HEAD(timer_event_list);
}

void el1_interrupt_enable()
{
    __asm__ __volatile__("msr daifclr, 0xf"); // umask all DAIF
}

void el1_interrupt_disable()
{
    __asm__ __volatile__("msr daifset, 0xf"); // mask all DAIF
}

void core_timer_enable()
{
    asm volatile(
        "mov x1, 1\n\t"
        "msr cntp_ctl_el0, x1\n\t" // cntp_ctl_el0[0]: enable, Control register for the EL1 physical timer.
                                   // cntp_tval_el0: Holds the timer value for the EL1 physical timer
    );
    *CORE0_TIMER_IRQ_CTRL |= 1 << 1; // enable
}

void timer_init(void)
{
    uart_puts("timer_init : enable timer\n");
    core_timer_enable();
    // core_timer_enable1();
    // core_timer_reset(value);
}
void core_timer_disable(void)
{
    // disable timer
    asm volatile("mov x1, 0 \n\t"
                 "msr cntp_ctl_el0, x1 \n\t");
    *CORE0_TIMER_IRQ_CTRL &= !(1 << 1);
}

void core_timer_hadler()
{
    if (list_empty(timer_event_list))
    {
        // disable timer interrupt
        core_timer_disable();
        return;
    }
    timer_event_callback((timer_event_t *)timer_event_list->next); // do callback and set new interrupt
}

////// Timer Multiplexing
void add_timer(void *callback, unsigned long long timeout, char *msgs)
{
    // 配置一個timer event 的空間
    timer_event_t *the_timer_event = kmalloc(sizeof(timer_event_t)); //
    // 用來儲存timer_event所有相關資訊
    the_timer_event->args = kmalloc(strlen(msgs) + 1);
    strcpy(the_timer_event->args, msgs);
    the_timer_event->interrupt_time = get_tick_plus_s(timeout);
    the_timer_event->callback = callback;
    INIT_LIST_HEAD(&the_timer_event->listhead);

    // 將timer event加入到 list當中
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
    // set interrupt to first event
    set_core_timer_interrupt_by_tick(((timer_event_t *)timer_event_list->next)->interrupt_time);
};

void timer_event_callback(timer_event_t *timer_event)
{
    list_del_entry((struct list_head *)timer_event); // delete the event in queue

    ((void (*)(char *))timer_event->callback)(timer_event->args); // call the event

    if (!list_empty(timer_event_list))
    {
        // 若還有其餘timer尚未處理完畢，則會將下一個timer事件的interrupt time設置起來
        set_core_timer_interrupt_by_tick(((timer_event_t *)timer_event_list->next)->interrupt_time);
    }
    else
    {
        // disable timer interrupt
        core_timer_disable();
    }
}

unsigned long long get_tick_plus_s(unsigned long long second)
{
    unsigned long long cntpct_el0 = 0;
    __asm__ __volatile__("mrs %0, cntpct_el0\n\t" : "=r"(cntpct_el0)); // tick auchor
    unsigned long long cntfrq_el0 = 0;
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t" : "=r"(cntfrq_el0)); // tick frequency
    return (cntpct_el0 + cntfrq_el0 * second);
}

// directly set timer interrupt time to a cpu tick  (directly)
void set_core_timer_interrupt_by_tick(unsigned long long tick)
{
    __asm__ __volatile__(
        "msr cntp_cval_el0, %0\n\t" // cntp_cval_el0 -> absolute timer
        : "=r"(tick));
}

void core_timer_intr_handler()
{

    uart_puts("Core timer interrupt ");
    put_int(2);

    //for (int i = 0; i < 100000000; i++)
    //{
       // i++;
     //   i--;
   // } // pi
    uart_puts(" seconeds\n");

    add_timer(core_timer_intr_handler, 2, "null");
}
