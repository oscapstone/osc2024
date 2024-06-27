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


static void
core_timer_set_tick(uint32_t duration)
{
    uint64_t freq = asm_read_sysregister(cntfrq_el0);    
    asm_write_sysregister(cntp_tval_el0, freq >> duration);
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


typedef struct timer_event {
    struct list_head    listhead;
    uint64_t            event_time;
    uint64_t            duration;
    uint64_t            expired_time;
    timer_event_cb      callback;
    byte_t              message[32];    // todo: pointer to a allocated space
    uint32_t            is_tick;
} timer_event_t;

typedef timer_event_t* timer_event_ptr_t;


static timer_event_ptr_t
timer_event_create(timer_event_cb cb, byteptr_t msg, uint64_t duration)
{
    timer_event_ptr_t event = (timer_event_ptr_t) kmalloc(sizeof(timer_event_t));
    INIT_LIST_HEAD(&event->listhead);
    event->event_time = core_timer_current_time();
    event->duration = duration;
    event->expired_time = event->event_time + duration;

    event->callback = cb;
    event->is_tick = 0;
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
        if (event->is_tick)
            core_timer_set_tick(event->duration);
        else 
            core_timer_set_timeout(event->expired_time - core_timer_current_time());
        core_timer_enable();
    }
}


static void
core_timer_callback_print_message(byteptr_t data)
{
    timer_event_ptr_t event = (timer_event_ptr_t) data;

    uart_printf("command time: %d\n", event->event_time);
    uart_printf("current time: %d\n", core_timer_current_time());
    uart_str("timer event message: ");
    uart_line(event->message);
}


static void
core_timer_add_event(timer_event_cb cb, byteptr_t msg, uint64_t duration)
{
    timer_event_ptr_t event = timer_event_create(cb, msg, duration);
    timer_event_queue_add(event);
}


void
core_timer_add_timeout_event(byteptr_t data, uint64_t duration)
{
    core_timer_add_event(core_timer_callback_print_message, data, duration);
}


void
core_timer_add_tick(timer_event_cb cb, byteptr_t msg, uint64_t duration)
{
    timer_event_ptr_t event = timer_event_create(cb, msg, duration);
    event->is_tick = 1;
    timer_event_queue_add(event);
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
    // uart_printf("core_timer_interrupt_handler\n");

    exception_l1_disable();

    timer_event_ptr_t event = (timer_event_ptr_t) (&timer_event_queue)->next;

    list_del_entry((list_head_ptr_t) event);

    if (event->is_tick) {
        timer_event_queue_add(event);
    }

    if (!list_empty(&timer_event_queue)) {
        timer_event_ptr_t first = (timer_event_ptr_t) (&timer_event_queue)->next;
        if (first->is_tick)
            core_timer_set_tick(first->duration);
        else 
            core_timer_set_timeout(first->expired_time - core_timer_current_time());
        core_timer_enable();
    }
    
    exception_l1_enable();

    event->callback((byteptr_t) event);
    if (!event->is_tick) { kfree((byteptr_t) event); }
}