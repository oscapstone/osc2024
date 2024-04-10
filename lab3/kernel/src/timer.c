#include "timer.h"
#include "uart1.h"
#include "heap.h"
#include "utils.h"

#define STR(x) #x
#define XSTR(s) STR(s)

struct list_head* timer_event_list;  // first head has nothing, store timer_event_t after it 

void timer_list_init(){
    INIT_LIST_HEAD(timer_event_list);
}

void core_timer_enable( ){
    __asm__ __volatile__(
        "mov x1, 1\n\t"
        "msr cntp_ctl_el0, x1\n\t" // enable

        "mrs x1, cntfrq_el0\n\t"
        "mov x2, 0x100000\n\t"
        "mul x1, x1, x2\n\t"    //set a big value prevent interrupt immediately
        // "mul x1, x1, %0\n\t"
        "msr cntp_tval_el0, x1\n\t" // set expired time

        "mov x2, 2\n\t"
        "ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t"
        "str w2, [x1]\n\t" // unmask timer interrupt
    // :"=r" (expired_time)
    );
}

void core_timer_handler(){
    // __asm__ __volatile__("mrs x10, cntpct_el0\n\t");
    // register unsigned long long cntpct_el0 asm ("x10");

    // __asm__ __volatile__("mrs x11, cntfrq_el0\n\t");
    // register unsigned long long cntfrq_el0 asm ("x11");

    // uart_puts("## Interrupt - el1_irq ## %d seconds after booting\n", cntpct_el0/cntfrq_el0);

    // __asm__ __volatile__(
    //     "mrs x0, cntfrq_el0\n\t"
    //     "mov x0, x0, LSL #1\n\t"   //set two second next time
    //     "msr cntp_tval_el0, x0\n\t"
    // );
    timer_event_callback((timer_event_t *)timer_event_list->next); // do callback and set new interrupt
}

void timer_event_callback(timer_event_t * timer_event){

    list_del_entry((struct list_head*)timer_event); // delete the event
    free(timer_event->args);  // free the arg space
    free(timer_event);
    ((void (*)(char*))timer_event-> callback)(timer_event->args);  // call the callback store in event

    //set interrupt to next time_event if existing
    if(!list_empty(timer_event_list))
    {
        set_core_timer_interrupt_by_tick(((timer_event_t*)timer_event_list->next)->interrupt_time);
    }
    else
    {
        set_core_timer_interrupt(1<<29);  // disable timer interrupt (set a very big value)
    }
}

