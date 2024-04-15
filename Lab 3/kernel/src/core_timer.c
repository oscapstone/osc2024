#include "core_timer.h"
#include "asm.h"
#include "mmio.h"
#include "uart.h"
#include "string.h"
#include "memory.h"
#include "exception.h"
#include "list.h"


/*
    core_timer_enable:
        mov x0, 1
        msr cntp_ctl_el0, x0 // enable
        
        mrs x0, cntfrq_el0
        msr cntp_tval_el0, x0 // set expired time

        mov x0, 2
        ldr x1, =CORE0_TIMER_IRQ_CTRL
        str w0, [x1] // unmask timer interrupt
*/


static void
core_timer_set_timeout(uint32_t duration)
{
   uint64_t freq = asm_read_sysregister(cntfrq_el0);
   asm_write_sysregister(cntp_tval_el0, freq * duration);
}


void
core_timer_enable()
{
   asm_write_sysregister(cntp_ctl_el0, 1);
   *CORE0_TIMER_IRQ_CTRL = (uint32_t) 2;       // unmask timer interrupt
}


void
core_timer_disable()
{
    asm_write_sysregister(cntp_ctl_el0, 0);     // disable
    *CORE0_TIMER_IRQ_CTRL = (uint32_t) 0;       // unmask timer interrupt
}


uint64_t
core_timer_current_time()
{
    uint64_t freq = asm_read_sysregister(cntfrq_el0);
    uint64_t cur_cnt = asm_read_sysregister(cntpct_el0);
    return cur_cnt / freq;
}


typedef void (*timer_event_cb)(byteptr_t);

typedef struct timer_event {
    struct list_head    listhead;
    uint64_t            event_time;
    uint64_t            expired_time;
    timer_event_cb      callback;
    byte_t              message[32];
} timer_event_t;

typedef timer_event_t* timer_event_ptr_t;


static timer_event_ptr_t
timer_event_create(timer_event_cb cb, byteptr_t msg, uint64_t duration)
{
    timer_event_ptr_t event = (timer_event_ptr_t) malloc(sizeof(timer_event_t));
    INIT_LIST_HEAD(&event->listhead);
    event->event_time = core_timer_current_time();
    event->expired_time = event->event_time + duration;
    event->callback = cb;
    str_ncpy(event->message, msg, 32);
    return event;
}


static LIST_HEAD(timer_event_queue);


static void 
timer_event_queue_add(timer_event_ptr_t event)
{    
    list_head_ptr_t curr = (&timer_event_queue)->next;

    while (!list_is_head(curr, &timer_event_queue) && ((timer_event_ptr_t) curr)->expired_time <= (event->expired_time)) {
        curr = curr->next;
    }
    list_add(&event->listhead, curr->prev);

    // if the new event is inserted at the front, update the timer
    if (list_is_first((list_head_ptr_t) event, &timer_event_queue)) {
        core_timer_set_timeout(event->expired_time - core_timer_current_time());
        core_timer_enable();
    }
}


static void
core_timer_callback_print_message(byteptr_t msg)
{
    uart_str("\ntimer message: ");
    uart_line(msg);
}


static void
core_timer_add_event(timer_event_cb cb, byteptr_t msg, uint64_t duration)
{
    timer_event_ptr_t event = timer_event_create(cb, msg, duration);
    timer_event_queue_add(event);
}


void
core_timer_add_timeout_event(byteptr_t messsage, uint64_t duration)
{
    core_timer_add_event(core_timer_callback_print_message, messsage, duration);
}


/*
    interrupt service routine
        1. run the first event of the list
        2. remove the first event
        3. check the next event of the list
        4. set the timer registers or clear them 
*/

void
core_timer_interrupt_handler()
{
    uart_line("core_timer_interrupt_handler");

    timer_event_ptr_t event = (timer_event_ptr_t) (&timer_event_queue)->next;

    event->callback(event->message);

    byte_t buffer[32];

    uint32_to_ascii(event->event_time, buffer);
    uart_str("command time: "); uart_line(buffer);
    
    uint32_to_ascii(core_timer_current_time(), buffer);
    uart_str("current  time: "); uart_line(buffer);

    list_del_entry((list_head_ptr_t) event);

    // todo: free(event);
    
    if (!list_empty(&timer_event_queue)) {
        timer_event_ptr_t first = (timer_event_ptr_t) (&timer_event_queue)->next;
        core_timer_set_timeout(first->expired_time - core_timer_current_time());
        core_timer_enable();
    }
}