#include "timer.h"
#include "mini_uart.h"
#include "c_utils.h"
#include "alloc.h"
#include "utils.h"
#include "exception.h"

timer_t *timer_list = 0;

void add_timer(timer_callback_t callback, void *data, unsigned int timeout) 
{
    timer_t *new_timer = simple_malloc(sizeof(timer_t));
    if(!new_timer) return;

    new_timer->callback = callback;
    new_timer->data = data;
    new_timer->timeout = timeout;
    new_timer -> next = 0;
    new_timer -> prev = 0;

    unsigned long long current_time, cntfrq;
    // read current count of the timer
    asm volatile("mrs %0, cntpct_el0" : "=r"(current_time)); 
    // read the frequency of the counter
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq));
    new_timer->expired_time = current_time + timeout * cntfrq;
    
    insert_timer_list(new_timer);
}

void insert_timer_list(timer_t *new_timer) 
{
    timer_t *current_timer = timer_list;
    disable_interrupt();

    if(!timer_list | (timer_list -> expired_time > new_timer -> expired_time)) 
    {
        // if the timer list is empty or the new timer will expire before the first timer in the list
        new_timer -> next = timer_list;
        new_timer -> prev = 0;
        if(timer_list)
            timer_list -> prev = new_timer;
        timer_list = new_timer;

        // cntp_cval_el0: A compared timer count. If cntpct_el0 >= cntp_cval_el0, interrupt the CPU core.
        // cntp_ctl_el0: Timer control register. Bit 0 is the enable bit. Set to 1 to enable the timer.
        // first ":" -> output operand, second ":" -> input operand
        asm volatile("msr cntp_cval_el0, %0" : : "r"(timer_list -> expired_time));
        asm volatile("msr cntp_ctl_el0, %0" : : "r"(1));

        enable_interrupt();
    }
    else
    {   
        // if the new timer will expire after the first timer in the list
        // find the right place in timer list to insert the new timer
        while(current_timer -> next && current_timer -> next -> expired_time < new_timer -> expired_time)
            current_timer = current_timer -> next;
        
        new_timer -> prev = current_timer;
        current_timer -> next = new_timer;
        if(current_timer -> next) {
            new_timer -> next = current_timer -> next;
            current_timer -> next -> prev = new_timer;
        }

        enable_interrupt();
    }
}

void set_timeout(const char* message, unsigned int timeout) 
{   
    // make sure when this function is finished, print_message can still access the message in the future.
    // cause the message is stored in the heap, it will not be destroyed after this function is finished.
    // but the one in stack will be destroyed after the function is finished.
    char* message_to_heap = simple_malloc(strlen(message)+1);
    if(!message_to_heap) 
    {
        uart_send_string("Error: failed to allocate memory for message in setTimeout.\r\n");
        return;
    }
    strncpy(message_to_heap, message, strlen(message)+1);

    // Check and enable timer if not already enabled
    if(!timer_list) // Enable the core timer interrupt
        put32(CORE0_TIMER_IRQ_CTRL, 2);

    add_timer(print_message, message_to_heap, timeout);
}

void print_message(void *data, unsigned int timeout) 
{
    char* message = data;
    unsigned long long current_time, cntfrq;
    // read current count of the timer
    asm volatile("mrs %0, cntpct_el0" : "=r"(current_time)); 
    // read the frequency of the counter
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq));
    // convert the current time to seconds
    unsigned int sec_since_boot = current_time / cntfrq;

    uart_send_string("\r\nTimeout message: ");
    uart_send_string(message);
    uart_send_string("\r\nTimeout: ");
    uart_send_string_int2hex(timeout);
    uart_send_string("\r\nCurrent time: ");
    uart_send_string_int2hex(sec_since_boot);
    uart_send_string(" seconds\r\n# ");
}


