#ifndef __TIMER_H__
#define __TIMER_H__

#define NR_TIMER                    (2)
#define TIMER_DISABLE               (0)
#define TIMER_ENABLE                (1)

#ifndef __ASSEMBLER__

#define CORE0_TIMER_IRQ_CTRL ((unsigned int *)0x40000040) // Core 0 Timers interrupt control: https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf

/**
 * Define a timer pool.
 * We can get the absolute time by add the value of the start_time and the value of the timeout.
*/
struct timer {
    unsigned char enable;  // 0: disable, 1: enable
    unsigned long start_time;
    unsigned long timeout;
    char message[64];
};

extern struct timer timer_pool[NR_TIMER];

/* Enable core timer interrupt and setup the next timer interrupt. */
extern void core_timer_enable(void);
unsigned long get_current_time(void);

/* Init timer struct for multiplex timer usage. */
void timer_init(void);
/* Get current time and compare with every timer inside the timer pool */
void timer_update(void);
/* Setup a new timer with timeout and message. */
int timer_set(unsigned long timeout, char *message);
/* Do timer_update in tasklet. */
void timer_tasklet(unsigned long data);

#endif // __ASSEMBLER__
#endif // __TIMER_H__