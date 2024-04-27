
#include "timer.h"
#include "delays.h"
#include "string.h"
#include "uart.h"
#include "stdint.h"
#include "demo.h"

volatile struct timer timer_pool[NR_TIMER];

/* Get current time by cntpct_el0 */
unsigned long get_current_time()
{
    unsigned long seconds;
    asm volatile(
        "mrs x0, cntpct_el0     \n\t"
        "mrs x1, cntfrq_el0     \n\t"
        "udiv %0, x0, x1        \n\t": "=r" (seconds));
    return seconds;
}

void timer_init()
{
    for (int i = 0; i < NR_TIMER; i++) {
        timer_pool[i].enable = TIMER_DISABLE;
        timer_pool[i].start_time = 0;
        timer_pool[i].timeout = 0;
        timer_pool[i].message[0] = '\0';
    }
    uart_puts("==== init: Timer\n");
}

int timer_set(unsigned long timeout, char *message)
{
    int i;
    for (i = 0; i < NR_TIMER; i++) {
        if (timer_pool[i].enable == TIMER_DISABLE) {
            timer_pool[i].enable = TIMER_ENABLE;
            timer_pool[i].start_time = get_current_time();
            timer_pool[i].timeout = timeout;
            strcpy((char *) timer_pool[i].message, message);
            return i;
        }
    }

    printf("No more timer available.\n");
    return -1;
}


/* Get current time and compare with every timer inside the timer pool */
void timer_update()
{
    /* Get the current time and compare with the timeout */
    unsigned long current = get_current_time();
    bool is_timer_enable = false;

    for (int i = 0; i < NR_TIMER; i++) {
        if (timer_pool[i].enable == TIMER_DISABLE)
            continue;
        is_timer_enable = true;
        unsigned long deadline = timer_pool[i].start_time + timer_pool[i].timeout;
        if (timer_pool[i].enable == TIMER_ENABLE && deadline < current) {
            // printf("\n==== One timer is timeout: %s\n", timer_pool[i].message);
            uart_puts("\n==== One timer is timeout: ");
            uart_puts((char *) timer_pool[i].message);
            uart_puts("\n");
            timer_pool[i].enable = TIMER_DISABLE;
            timer_pool[i].timeout = 0;
            timer_pool[i].message[0] = '\0';
        }
    }
    if (is_timer_enable) {
        // printf("Current time: %d\n", current);
        uart_puts("Current time: ");
        uart_hex(current);
        uart_puts("\n");
    }
}

/* Timer tasklet: do timer_update(). */
void timer_tasklet(unsigned long data)
{
#ifdef DEMO
    uart_puts("Into timer_tasklet\n");
#endif

    timer_update();

#ifdef DEMO
    // wait_cycles(50000000); // For raspi 3b+, this delay is very long.
    uart_puts("Exit timer_tasklet\n");
#endif
}
