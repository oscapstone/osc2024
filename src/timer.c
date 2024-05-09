
#include "timer.h"
#include "delays.h"
#include "string.h"
#include "uart.h"
#include "stdint.h"
#include "demo.h"

struct timer timer_pool[NR_TIMER];

/* Get current time (seconds) by cntpct_el0 */
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

    printf("timer_set: timeout %d ,message %s\n", timeout, message); // Without this, the message will be empty. gcc's fault.
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
            printf("\n==== One timer is timeout: %s\n", timer_pool[i].message);
            timer_pool[i].enable = TIMER_DISABLE;
            timer_pool[i].timeout = 0;
            timer_pool[i].message[0] = '\0';
        }
    }
    if (is_timer_enable)
        printf("Current time: %d\n", current);
}

/* Timer tasklet: do timer_update(). */
void timer_tasklet(unsigned long data)
{
    timer_update();
}