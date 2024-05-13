#include "fork.h"
#include "alloc.h"
#include "schedule.h"
#include "io.h"
#include "mm.h"

extern void ret_from_fork(void);
extern int nr_tasks;
extern struct task_struct * task[NR_TASKS];
extern void memzero(unsigned long begin, unsigned long len);

int copy_process(
	unsigned long flags,
	unsigned long fn, 
	unsigned long arg,
	unsigned long stack)
{
	preempt_disable();
	struct task_struct *p;

	int pid = -1;
	for(pid = 0; pid< NR_TASKS; pid++)
		if(task[pid] == NULL) break;
	if(pid == -1)
	{
		printf("\r\n[ERROR] Thread limit exceeded");
		goto finish;
	}
	// p = (struct task_struct *) balloc(sizeof(struct task_struct));
	p = (struct task_struct *) balloc(THREAD_SIZE);
	printf("\r\n[INFO] Process allocated at "); printf_hex((unsigned long)p);
	if (!p)
		return -1;

	struct pt_regs *childregs = task_pt_regs(p);
	memzero_asm((unsigned long)childregs, sizeof(struct pt_regs));
	memzero_asm((unsigned long)&p->cpu_context, sizeof(struct cpu_context));

	if(flags & PF_KTHREAD) // is a kernel thread
	{
		p->cpu_context.x19 = fn;
		p->cpu_context.x20 = arg;
	} 
	else
	{
		struct pt_regs *cur_regs = task_pt_regs(current);
		// printf("\r\ncurrent pid: "); printf_int(current->pid);
		for(int i=0; i<sizeof(struct pt_regs); i++)
			((char*)childregs)[i] = ((char*)cur_regs)[i];

		childregs->regs[0] = 0; // child process return value
		// next sp for the new task is set to the top of the new user stack
		// save the stack pointer to cleanup the stack when task finishes
		childregs->sp =  stack + THREAD_SIZE;
		p->stack = stack;
	}
	p->flags = flags;
	p->priority = current->priority;
	p->state = TASK_RUNNING;
	p->counter = p->priority;
	p->preempt_count = 1; //disable preemtion until schedule_tail

	
	p->cpu_context.pc = (unsigned long)ret_from_fork;   // for the first time, pc is set to ret_from_fork
	// p->cpu_context.sp = (unsigned long)p + THREAD_SIZE;
	p->cpu_context.sp = (unsigned long)childregs;
	
	task[pid] = p;
	p->pid = pid;
	printf("\r\n[INFO] Process created with pid: "); printf_int(pid);
finish:
	preempt_enable();
	return pid;
}

int move_to_user_mode(unsigned long pc)
{
    struct pt_regs *regs = task_pt_regs(current);	// get current process state
    memzero_asm((unsigned long)regs, sizeof(struct pt_regs));

    regs->pc = pc;	// point to the function that need to be executed in user mode
    regs->pstate = PSR_MODE_EL0t;	// EL0t is user state

    unsigned long stack = (unsigned long)balloc(THREAD_SIZE); //allocate new user stack
	memzero_asm(stack, THREAD_SIZE);

	// printf("\r\n[INFO] User stack from: "); printf_hex(stack); printf(" to "); printf_hex(stack + THREAD_SIZE);
    if (!stack) {	// if stack allocation failed
        return -1;
    }

	// allocate stack for user process
	// set sp to the top of the stack
    regs->sp = stack + THREAD_SIZE;
    current->stack = stack;
    return 0;
}


// calculate the location of pt_regs in the task_struct
// task_struct is allocated at the end of the stack
// pt_regs is allocated at the top of the task_struct
struct pt_regs * task_pt_regs(struct task_struct *tsk){
	unsigned long p = (unsigned long)tsk + THREAD_SIZE - sizeof(struct pt_regs);
	return (struct pt_regs *)p;
}

void kp_user_mode(unsigned long func){ // Kernel process(func pass by argu) to user mode
    printf("\r\nKernel process started. Switching to user mode...");
    int err = move_to_user_mode(func);
    if (err < 0){
        printf("Error while moving process to user mode\n\r");
    }
}