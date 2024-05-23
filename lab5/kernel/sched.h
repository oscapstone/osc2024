#ifndef _SCHED_H_
#define _SCHED_H_

#define THREAD_CPU_CONTEXT			0 	

#define STACK_SIZE 4096

#define PF_KTHREAD 0x2

#ifndef __ASSEMBLER__
struct cpu_context {
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
	unsigned long long fp; // x29
	unsigned long long sp;
	unsigned long long pc; // x30
};

enum task_state {
    TASK_RUNNING,
    TASK_ZOMBIE,
};

typedef struct task_struct {
    struct cpu_context context;
    long state;
    long counter;
    long priority;
    long preempt_count;
    unsigned long stack;
    unsigned long flags;
    int pid;
    struct task_struct *prev, *next;
} task_struct_t;

#define INIT_TASK \
/*cpu_context*/	{ {0,0,0,0,0,0,0,0,0,0,0,0,0}, \
/* state etc */	0, 0, 1, 0, 0, PF_KTHREAD, 0, 0, 0 \
}

unsigned long get_new_pid();
void enqueue_run_queue(task_struct_t *);
void delete_run_queue(task_struct_t *);
void sched_init();
void timer_tick();
void preempt_disable();
void preempt_enable();
void exit_process();
void kill_process(long pid);
void root_task();

void print_task_list();

// fork
int copy_process(unsigned long flags, unsigned long fn, unsigned long arg,
                 unsigned long stack);
int move_to_user_mode(unsigned long pc);

struct pt_regs {
    unsigned long regs[31];
    unsigned long sp;
    unsigned long pc;
    unsigned long pstate;
};

struct pt_regs *task_pt_regs(task_struct_t *tsk);

#define PSR_MODE_EL0t 0x00000000;

task_struct_t *get_current();

#endif
#endif
