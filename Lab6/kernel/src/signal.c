#include "signal.h"
#include "arm/sysregs.h"
#include "fork.h"
#include "irq.h"
#include "math.h"
#include "mm.h"
#include "slab.h"
#include "utils.h"

void sigkill_default_handler(void);

static void (*default_sig_handler[NR_SIGNAL])(void) = {
    &sigkill_default_handler, &sigkill_default_handler,
    &sigkill_default_handler, &sigkill_default_handler,
    &sigkill_default_handler, &sigkill_default_handler,
    &sigkill_default_handler, &sigkill_default_handler,
    &sigkill_default_handler, [SIGKILL] = &sigkill_default_handler};

static inline int invalid_sig(int SIGNAL)
{
    if (SIGNAL < 0 || SIGNAL >= NR_SIGNAL)
        return 1;
    return 0;
}

int reg_sig_handler(struct task_struct* task, int SIGNAL, void (*sig_handler)())
{
    if (invalid_sig(SIGNAL))
        return 0;
    task->sig_handler[SIGNAL] = sig_handler;
    return 1;
}


int recv_sig(struct task_struct* task, int SIGNAL)
{
    if (invalid_sig(SIGNAL))
        return 0;
    task->sig_pending |= (1 << SIGNAL);
    return 1;
}

void handle_sig(void)
{
    if (current_task->sig_handling)
        return;

    current_task->sig_handling = 1;

    while (current_task->sig_pending) {
        long pending = current_task->sig_pending;
        pending = get_lowest_set_bit(pending);
        int SIGNAL = LOG2LL(pending);

        // default signal handler
        if (!current_task->sig_handler[SIGNAL]) {
            default_sig_handler[SIGNAL]();
            return;
        }

        // registered signal handler
        if (!current_task->sig_stack)
            current_task->sig_stack = (void*)allocate_user_pages(
                current_task, STK, USR_SIG_STK_ADDR, USR_SIG_STK_SZ, 0);

        struct pt_regs* sig_regs = task_sig_regs(current_task);
        struct pt_regs* regs = task_pt_regs(current_task);
        *sig_regs = *regs;

        regs->pc = (unsigned long)current_task->sig_handler[SIGNAL];
        regs->pstate = SPSR_EL0t;
        regs->sp = USR_SIG_STK_ADDR + USR_SIG_STK_SZ - sizeof(struct pt_regs);
        regs->regs[30] = (unsigned long)sig_return;

        current_task->sig_pending ^= pending;
    }

    current_task->sig_handling = 0;
}

void sigkill_default_handler(void)
{
    exit_process();
}

struct pt_regs* task_sig_regs(struct task_struct* task)
{
    if (!task->sig_stack)
        return NULL;
    return (struct pt_regs*)((unsigned long)task->sig_stack + USR_SIG_STK_SZ -
                             sizeof(struct pt_regs));
}

void do_sig_return(void)
{
    struct pt_regs* regs = task_pt_regs(current_task);
    struct pt_regs* sig_regs = task_sig_regs(current_task);
    *regs = *sig_regs;
}
