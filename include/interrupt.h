/**
 * Ref: linux/interrupt.h
*/
#ifndef __INTERUPT_H__
#define __INTERUPT_H__

enum
{
	HI_SOFTIRQ=0,
	TIMER_SOFTIRQ,
	NET_TX_SOFTIRQ,
	NET_RX_SOFTIRQ,
	BLOCK_SOFTIRQ,
	IRQ_POLL_SOFTIRQ,
	TASKLET_SOFTIRQ,
	SCHED_SOFTIRQ,
	HRTIMER_SOFTIRQ,
	RCU_SOFTIRQ,    /* Preferable RCU should always be the last softirq */

	NR_SOFTIRQS
};

struct softirq_action {
    void (*action)(void *data); // the function pointer should keep the data it needs.
};



/*
 * Ref: kernel/time/timer.c
 * This function runs timers and the timer-tq in bottom half context.

static __latent_entropy void run_timer_softirq(struct softirq_action *h)
{
	struct timer_base *base = this_cpu_ptr(&timer_bases[BASE_STD]);

	__run_timers(base);
	if (IS_ENABLED(CONFIG_NO_HZ_COMMON))
		__run_timers(this_cpu_ptr(&timer_bases[BASE_DEF]));
}
 */


#endif // __INTERUPT_H__
