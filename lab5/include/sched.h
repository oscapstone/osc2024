#ifndef SCHED_H
#define SCHED_H

#define THREAD_CPU_CONTEXT		0x10 		// offset of cpu_context in task_struct

#ifndef __ASSEMBLER__

#include "list.h"

#define THREAD_SIZE				 4096

#define TASK_RUNNING				0
#define TASK_STOPPED                1

extern struct task_struct *current;
extern struct list_head task_head_start;
extern uint32_t nr_tasks;

struct cpu_context {
	struct list_head task_head;
	unsigned long x19;
	unsigned long x20;
	unsigned long x21;
	unsigned long x22;
	unsigned long x23;
	unsigned long x24;
	unsigned long x25;
	unsigned long x26;
	unsigned long x27;
	unsigned long x28;
	unsigned long fp;
	unsigned long sp;
	unsigned long pc;
};

struct task_struct {
	struct cpu_context cpu_context;
	long state;	
	long counter;
	long priority;
	long preempt_count;
	long pid;
};

void show_task_head(void);
void schedule(void);
void schedule_tail(void);
void timer_tick(char *);
void preempt_disable(void);
void preempt_enable(void);
void switch_to(struct task_struct* next);
extern void cpu_switch_to(struct task_struct* prev, struct task_struct* next);
void idle(void);
void kill_zombies(void);
void exit_process(void);

#define INIT_TASK(my_task) \
/*cpu_context*/	{ {{{&my_task.cpu_context.task_head, &my_task.cpu_context.task_head}},0,0,0,0,0,0,0,0,0,0,0,0,0}, \
/* state etc */	0,0,1,0 \
}

#endif
#endif