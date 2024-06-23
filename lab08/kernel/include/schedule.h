#ifndef __SCHEDULE_H__
#define __SCHEDULE_H__

#define THREAD_CPU_CONTEXT			0 		// offset of cpu_context in task_struct 


#ifndef __ASSEMBLER__

#define THREAD_SIZE				4096

#define NR_TASKS				64 

#define FIRST_TASK task[0]
#define LAST_TASK task[NR_TASKS-1]

#define TASK_RUNNING				0
#define TASK_ZOMBIE				    1
#define TASK_STOPPED				2

#define PF_KTHREAD 					2

// #define MAX_PATH_LEN				256

extern struct task_struct *current;
extern struct task_struct * task[NR_TASKS];
extern int nr_tasks;

#include "vfs.h"

struct cpu_context {
	unsigned long long x19;
	unsigned long long x20;
	unsigned long long x21;
	unsigned long long x22;
	unsigned long long x23;
	unsigned long long x24;
	unsigned long long x25;
	unsigned long long x26;
	unsigned long long x27;
	unsigned long long x28;
	unsigned long long fp; // x29
	unsigned long long sp;
	unsigned long long pc; // x30
};

struct task_struct {
	struct cpu_context cpu_context;
	long state;	
	long counter;
	long priority;
	long preempt_count;
	unsigned long stack;
	unsigned long flags;
	int pid;
	char cwd[MAX_PATH_LEN];
	struct file_descriptor_table fd_table;
};

extern void sched_init(void);
extern void schedule(void);
extern void timer_tick(void);
extern void preempt_disable(void);
extern void preempt_enable(void);
extern void switch_to(struct task_struct* next);
extern void cpu_switch_to(struct task_struct* prev, struct task_struct* next);
extern void enable_irq(void);
extern void disable_irq(void);
extern void kill_zombies(void);
extern void exit_process(void);

#define INIT_TASK \
/*cpu_context*/	{ {0,0,0,0,0,0,0,0,0,0,0,0,0}, \
/* state etc */	0,0,1, 0, 0, PF_KTHREAD, 0, \
/* cwd */ "/", \
/*fd table*/ {0, {0}} \
}

#endif
#endif