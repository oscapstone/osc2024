/**
 * Ref: linux/interrupt.h
*/
#ifndef __INTERUPT_H__
#define __INTERUPT_H__

/* Tasklet priority */
enum
{
	NO_TASKLET = 0,
	UART_TASKLET = 1,
	TIMER_TASKLET = 2, // timer has highest priority
};

struct softirq_action {
    void (*action)(void); // the function pointer should keep the data it needs.
};

/* Ref: linux/interrupt.h */
struct tasklet_struct {
	struct tasklet_struct *next;
	unsigned long state; // Use it to store priority.
	void (*func)(unsigned long);
	volatile unsigned long data;
};

/* Ref: kernel/softirq.c */
struct tasklet_head {
	struct tasklet_struct *head;
	struct tasklet_struct **tail;
};

extern struct tasklet_struct tl_pool[2]; // I don't want to dynamically allocate memory for tasklet. So I use a fixed size pool.
extern struct tasklet_head tl_head; /* In linux, every cpu has its own tasklet_haed. */
extern volatile unsigned long cur_tl_priority; /* In osc2024 lab3, we use it to store the priority of current tasklet. */

void do_tasklet(void);
void tasklet_add(struct tasklet_struct *tl);
void tasklet_init(void);


/* Softirq not implemented in this project for now. */
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
extern struct softirq_action softirq_vec[NR_SOFTIRQS]; // May not be used in osdi project.
void softirq_init(void);
void open_softirq(int nr, void (*action)(void));


#endif // __INTERUPT_H__
