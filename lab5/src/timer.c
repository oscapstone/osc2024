#include "timer.h"
#include "mini_uart.h"
#include "string.h"
#include "alloc.h"
#include "c_utils.h"
#include "exception.h"

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

    unsigned long long current_time, cntfrq;
    asm volatile("mrs %0, cntpct_el0" : "=r"(current_time));
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq));

    timer->timeout = current_time + timeout * cntfrq;
    
    add_timer(timer);
}

void add_timer(timer_t *timer) {
    timer_t *current = timer_head;
    el1_interrupt_disable();

    // if the timer list is empty or the new timer is the first to expire
    if(!timer_head | (timer_head -> timeout > timer -> timeout)) {
        timer -> next = timer_head;
        timer -> prev = 0;
        if(timer_head) {
            timer_head -> prev = timer;
        }
        timer_head = timer;

        asm volatile("msr cntp_cval_el0, %0" : : "r"(timer_head -> timeout));
        asm volatile("msr cntp_ctl_el0, %0" : : "r"(1));

        el1_interrupt_enable();
        return;
    }

    // find the correct position to insert the new timer
    while(current -> next && current -> next -> timeout < timer -> timeout) {
        current = current -> next;
    }

    timer -> next = current -> next;
    timer -> prev = current;
    if(current -> next) {
        current -> next -> prev = timer;
    }
    current -> next = timer;

    el1_interrupt_enable();
}