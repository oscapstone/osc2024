#include "fork.h"
#include "alloc.h"
#include "schedule.h"
#include "io.h"

extern void ret_from_fork(void);
extern int nr_tasks;
extern struct task_struct * task[NR_TASKS];

int copy_process(unsigned long fn, unsigned long arg)
{
	preempt_disable();
	struct task_struct *p;

	int pid = -1;
	for(pid = 0; pid< NR_TASKS; pid++)
		if(task[pid] == NULL) break;
	if(pid == -1)
	{
		printf("\r\nERROR] Thread limit exceeded");
		goto finish;
	}
	p = (struct task_struct *) balloc(sizeof(struct task_struct));
	if (!p)
		return 1;
	p->priority = current->priority;
	p->state = TASK_RUNNING;
	p->counter = p->priority;
	p->preempt_count = 1; //disable preemtion until schedule_tail

	p->cpu_context.x19 = fn;
	p->cpu_context.x20 = arg;
	p->cpu_context.pc = (unsigned long)ret_from_fork;   // for the first time, pc is set to ret_from_fork
	p->cpu_context.sp = (unsigned long)p + THREAD_SIZE;
	
	task[pid] = p;
	p->pid = pid;
finish:
	preempt_enable();
	return 0;
}