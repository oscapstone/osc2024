#include "task.h"
#include "memory.h"
#include "exception.h"
#include "entry.h"
#include "uart.h"
#include "list.h"
#include "arm/sysregs.h"

#define THREAD_SIZE     0x1000


void
task_kill(task_ptr tsk, int32_t status)
{ 
    tsk->state = TASK_STOPPED;
    tsk->exitcode = status;

    if (tsk->user_stack)
        frame_release(tsk->user_stack);
    if (tsk->frame_count)
        frame_release_block(tsk->user_prog, tsk->frame_count);
}


int32_t     task_count = 0;


ptregs_ptr
task_pt_regs(task_ptr tsk)
{
	uint64_t p = (uint64_t) tsk + THREAD_SIZE - sizeof(pt_regs_t);
	return (ptregs_ptr) p;  
}


void
task_init(task_ptr tsk)
{
    memory_zero((byteptr_t) tsk, sizeof(task_t));
    memory_zero((byteptr_t) &tsk->cpu_context, sizeof(cpu_context_t));
    ptregs_ptr childregs = task_pt_regs(tsk);
    memory_zero((byteptr_t) childregs, sizeof(pt_regs_t));
    tsk->cpu_context.sp = (uint64_t) childregs;
    tsk->cpu_context.lr = (uint64_t) ret_from_fork;
    INIT_LIST_HEAD((list_head_ptr_t) tsk);
    tsk->priority = 1;
}


void
task_reset(task_ptr tsk, uint64_t fn, uint64_t arg)
{
    int32_t pid = tsk->pid;
    task_init(tsk);
    tsk->cpu_context.x19 = fn;
    tsk->cpu_context.x20 = arg;
    tsk->state = TASK_RUNNING;
    tsk->pid = pid;
}


task_ptr
task_create(uint64_t fn, uint64_t arg)
{ 
    task_ptr p = (task_ptr) frame_alloc(1);
    if (!p) { return 0; }
    task_reset(p, fn, arg);
    p->pid = ++task_count;

    return p;
}


task_ptr
task_fork(task_ptr parent)
{
    // 1. build child task
    task_ptr child = (task_ptr) frame_alloc(1);
    if (!child) { return 0; }
    // memory_copy((byteptr_t) child, (byteptr_t) parent, sizeof(task_t));
    memory_zero((byteptr_t) child, sizeof(task_t));
    child->foreground = 0;
    child->sig9_handler = parent->sig9_handler;

    // 2. copy trapframe
    ptregs_ptr currentregs = task_pt_regs(parent);
    ptregs_ptr childregs = task_pt_regs(child);
    memory_copy((byteptr_t) childregs, (byteptr_t) currentregs, sizeof(pt_regs_t));

    // 3. set the kernel stack of the child  
    child->cpu_context.x19 = 0;
    child->cpu_context.x20 = 0;
    child->cpu_context.sp = (uint64_t) childregs;
    child->cpu_context.lr = (uint64_t) ret_from_fork;
    child->priority = 1;

    // 4. assign the child a new id
    child->pid = ++task_count;

    // 5. copy user stack
    if (parent->user_stack) {
        child->user_stack = frame_alloc(1);
        memory_copy((byteptr_t) child->user_stack, (byteptr_t) parent->user_stack, THREAD_SIZE);
    }

    // 6. copy user program
    if (parent->user_prog) {
        child->user_prog_size = parent->user_prog_size;
        child->frame_count = parent->frame_count;
        child->user_prog = frame_alloc(parent->frame_count);
        memory_copy((byteptr_t) child->user_prog, (byteptr_t) parent->user_prog, parent->user_prog_size);
    }

    // 7. calculate the offsets of user mode
    childregs->regs[30] = (uint64_t) child->user_prog + (currentregs->regs[30] - (uint64_t) parent->user_prog);    // link return
    childregs->sp = (uint64_t) child->user_stack + (currentregs->sp - (uint64_t) parent->user_stack);
    childregs->pc = (uint64_t) child->user_prog + (currentregs->pc - (uint64_t) parent->user_prog);
    // childregs->pstate = 0;
    
    // 8. returned value
    childregs->regs[0] = 0;

    return child;
}


int32_t
task_to_user_mode(task_ptr tsk, uint64_t pc)
{
    uart_printf("[DEBUG] task_to_user_mode - pc: 0x%x\n", pc);

    // 1. build a user stack
    uint64_t stack = (uint64_t) frame_alloc(1);
	if (!stack) { return -1; }

    tsk->user_stack = (byteptr_t) stack;

    // 2. prepare cpu state for user mode
    ptregs_ptr regs = task_pt_regs(tsk);
    memory_zero((byteptr_t) regs, sizeof(pt_regs_t));

    regs->sp = stack + THREAD_SIZE;
    regs->pc = pc;
    regs->pstate = PSR_MODE_EL0t;   // 
    return 0;
}


void
task_add_signal(task_ptr tsk, int32_t number, byteptr_t handler)
{
    // TODO: build a signal handler table
    if (number == 9) tsk->sig9_handler = (signal_handler) handler;
}

static void sigcall_return()
{
    asm volatile("mov x8, 10");
    asm volatile("svc 0");
}


void
task_to_signal_handler(task_ptr tsk, int32_t number)
{
    if (number == 9 && tsk->sig9_handler) {
        ptregs_ptr tsk_regs = task_pt_regs(tsk);
        tsk->signal_stack = (byteptr_t) frame_alloc(1);
        tsk->ptregs_backup = (ptregs_ptr) kmalloc(sizeof(pt_regs_t));
        memory_copy((byteptr_t) tsk->ptregs_backup, (byteptr_t) tsk_regs, sizeof(pt_regs_t));

        tsk_regs->regs[30] = (uint64_t) &sigcall_return;
        tsk_regs->sp = (uint64_t) tsk->signal_stack + THREAD_SIZE;
        tsk_regs->pc = (uint64_t) tsk->sig9_handler;
    }
}


void
task_ret_from_signal_handler(task_ptr tsk)
{
    ptregs_ptr tsk_regs = task_pt_regs(tsk);
    memory_copy((byteptr_t) tsk_regs, (byteptr_t) tsk->ptregs_backup, sizeof(pt_regs_t));

    frame_release((byteptr_t) tsk->signal_stack);
    kfree((byteptr_t) tsk->ptregs_backup);

    tsk->signal_stack = 0;
    tsk->ptregs_backup = 0;
}