#include "timer.h"
#include "mini_uart.h"
#include "string.h"
#include "alloc.h"
#include "c_utils.h"
#include "exception.h"
#include "thread.h"
#include "mini_uart.h"

timer_t *timer_head = 0;

void print_message(void *data) {
    char* message = data;
    unsigned long long current_time, cntfrq;
    asm volatile("mrs %0, cntpct_el0" : "=r"(current_time));
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq));

    unsigned int sec = current_time / cntfrq;

    uart_send_string("\nTimeout message: ");
    uart_send_string(message);
    uart_send_string(" at ");
    uart_hex(sec);
    uart_send_string(" seconds\n# ");
}

void set_timeout(char* message, unsigned long long timeout) {
    char* message_copy = (char*)kmalloc(strlen(message)+1);
    strncpy_(message_copy, message, strlen(message)+1);
    if(!message_copy) return;

    if(!timer_head) {
        // enable timer
        *CORE0_TIMER_IRQ_CTRL = 2;
    }

    create_timer(print_message, message_copy, timeout);
}

void create_timer(
    timer_callback_t callback, 
    void *data, 
    unsigned long long timeout
) {
    timer_t *timer = (timer_t*)kmalloc(sizeof(timer_t));
    if(!timer) return;

    timer->callback = callback;
    timer->data = data;
    timer->next = 0;

    unsigned long long current_time, cntfrq;
    asm volatile("mrs %0, cntpct_el0" : "=r"(current_time));
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq));

    timer->timeout = current_time + timeout * cntfrq;
    
    add_timer(&timer);
}

void add_timer(timer_t **timer) {
    el1_interrupt_disable();
    // uart_send_string("add timer\n");
    // if the timer list is empty or the new timer is the first to expire
    if(!timer_head) uart_send_string("timer_head is null\n");
    if(!timer_head || (timer_head -> timeout > (*timer) -> timeout)) {
        (*timer) -> next = timer_head;
        timer_head = *timer;
        asm volatile("msr cntp_ctl_el0, %0" : : "r"(1));
        asm volatile("msr cntp_cval_el0, %0" : : "r"(timer_head -> timeout));
        el1_interrupt_enable();
        return;
    }

    timer_t *current = timer_head;

    // find the correct position to insert the new timer
    while(current -> next && current -> next -> timeout < (*timer) -> timeout) {
        current = current -> next;
    }

    (*timer) -> next = current -> next;
    current -> next = (*timer);

    el1_interrupt_enable();
}


void create_timer_freq_shift(
    timer_callback_t callback, 
    void *data, 
    unsigned long long shift
) {
    timer_t *timer = (timer_t*)kmalloc(sizeof(timer_t));
    if(!timer) return;

    timer->callback = callback;
    timer->data = data;

    unsigned long long current_time, cntfrq;
    asm volatile("mrs %0, cntpct_el0" : "=r"(current_time));
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq));

    timer->timeout = current_time + (cntfrq >> shift);
    // uart_send_string("timeout: ");
    // uart_hex(timer->timeout);
    // uart_send_string("\n");
    add_timer(&timer);
}

void schedule_task(void* data) {
    // uart_send_string("Schedule task\n");
    create_timer_freq_shift(schedule_task, 0, 5);
}