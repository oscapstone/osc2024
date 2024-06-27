#ifndef __TASK_H__
#define __TASK_H__

#include "type.h"
#include "list.h"


typedef struct pt_regs {
	uint64_t regs[31];
	uint64_t sp;
	uint64_t pc;
	uint64_t pstate;
} pt_regs_t;
typedef pt_regs_t * ptregs_ptr;


typedef void (*signal_handler)(int);

typedef struct signal {
    list_head_t     list;
    signal_handler  handler;
    uint32_t        sig_num;
} signal_t;
typedef signal_t * signal_ptr;



#define TASK_SIZE             0x1000

typedef struct cpu_context {
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
    unsigned long fp;   // x30
    unsigned long sp;   // x31
    unsigned long lr;
} cpu_context_t;


typedef enum {
    TASK_RUNNING,
    TASK_WAITING,
    TASK_STOPPED
} state_t;


typedef struct task {
	list_head_t 	list;
    cpu_context_t 	cpu_context;
    int32_t 		pid;
	uint32_t		exitcode;
    state_t 		state;

    byteptr_t       user_stack;
    byteptr_t       user_prog;
    uint32_t        user_prog_size;
    uint32_t        frame_count;
    uint32_t        foreground;

    int32_t         priority;
    int32_t         preempt_count;
    int32_t         counter;

    signal_handler  sig9_handler;
    ptregs_ptr      ptregs_backup;
    byteptr_t       signal_stack;

} task_t;

typedef task_t * task_ptr;




extern int32_t task_count;


task_ptr    task_create(uint64_t fn, uint64_t arg);
void        task_init(task_ptr tsk);
void        task_kill(task_ptr tsk, int32_t status);
void        task_reset(task_ptr tsk, uint64_t fn, uint64_t arg);
task_ptr    task_fork(task_ptr tsk);
int32_t     task_to_user_mode(task_ptr tsk, uint64_t pc);
void        task_add_signal(task_ptr tsk, int32_t number, byteptr_t handler);

void        task_to_signal_handler(task_ptr tsk, int32_t number);
void        task_ret_from_signal_handler(task_ptr tsk);

#endif