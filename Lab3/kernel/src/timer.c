#include "timer.h"
#include "mini_uart.h"

static unsigned int seconds = 0;

unsigned int get_seconds(void)
{
    return seconds;
}

void set_seconds(unsigned int s)
{
    seconds = s;
}

void set_core_timer_timeout(void)
{
    unsigned long freq = 0;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));
    asm volatile("msr cntp_tval_el0, %0" ::"r"(seconds * freq));
}


void core_timer_handle_irq(void)
{
    set_core_timer_timeout();
    uart_send_string("Core timer interrupt\n");

    unsigned long current_time = get_current_time();

    uart_send_dec(current_time);
    uart_send_string(" seconds since boot\n");
}
