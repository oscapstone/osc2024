#include "uart.h"
#include "sched.h"
#include "syscall.h"
#include "exec.h"
#include "exception.h"
#include "kernel.h"
#include "mbox.h"
#include "initrd.h"
#include "mm.h"
#include "string.h"

/* The definition of system call handler function */
syscall_t sys_call_table[SYSCALL_NUM] = {
    sys_get_taskid,
    sys_uart_read,
    sys_uart_write,
    sys_exec,
    sys_fork,
    sys_exit,
    sys_mbox_call,
    sys_kill,
    sys_signal,
    sys_sigkill,
    sys_sigreturn
};

/* As a handler function for svc, setup the return value to trapframe */
int sys_get_taskid(struct trapframe *trapframe)
{
#ifdef DEBUG_SYSCALL
    printf("[sys_get_taskid] Task %d get task id\n", current->task_id);
#endif
    trapframe->x[0] = current->task_id;
    return SYSCALL_SUCCESS;
}

/* uart read system call: x0: buffer address, x1: buffer size */
int sys_uart_read(struct trapframe *trapframe)
{
#ifdef DEBUG_SYSCALL
    printf("[sys_uart_read] Task %d read\n", current->task_id);
#endif
    /* x0: buffer address, x1: buffer size */
    enable_interrupt(); // Enable interrupt, or the kernel will be stuck here.
    char *buff_addr = (char *) trapframe->x[0];
    size_t buff_size = (size_t) trapframe->x[1];
    for (int i = 0; i < buff_size; i++)
        buff_addr[i] = uart_getc();
    trapframe->x[0] = buff_size; // return the number of bytes read
#ifdef DEBUG_SYSCALL
    printf("[sys_uart_read] Task %d read end\n", current->task_id);
#endif
    return SYSCALL_SUCCESS;
}

/* uart write system call: x0: buffer address, x1: buffer size */
int sys_uart_write(struct trapframe *trapframe)
{
#ifdef DEBUG_SYSCALL
    printf("[sys_uart_write] Task %d write\n", current->task_id);
#endif
    /* x0: buffer address, x1: buffer size */
    char *buff_addr = (char *) trapframe->x[0];
    size_t buff_size = (size_t) trapframe->x[1];
    for (int i = 0; i < buff_size; i++)
        uart_send(buff_addr[i]); // show the string from buffer given by user.
    trapframe->x[0] = buff_size; // return the number of bytes read
    return SYSCALL_SUCCESS;
}

/* sys_exec: the function won't return? */
int sys_exec(struct trapframe *trapframe)
{
#ifdef DEBUG_SYSCALL
    printf("[sys_exec] Task %d exec\n", current->task_id);
#endif
    /* Find the program in initrd */
    initrd_usr_prog((char *) trapframe->x[0]);
    trapframe->x[0] = 0; // 0 means success
    return SYSCALL_SUCCESS;
}

int sys_fork(struct trapframe *trapframe)
{
#ifdef DEBUG_SYSCALL
    printf("[sys_fork] Task %d fork\n", current->task_id);
#endif
    unsigned int child_task_id, kstack_offset, i;
    struct task_struct *child_task;
    struct trapframe *child_trapframe;
    unsigned long *child_pgd, *parent_pgd;
    char *child_stack, *parent_stack;

    child_task_id = find_empty_task();
    child_task = &task_pool[child_task_id];

    /* Copy the content of parent's task_struct and update it based on child task */
    *child_task = *current;
    child_task->task_id = child_task_id;
    child_task->counter = child_task->priority;
    child_task->tss.lr = (uint64_t) &exit_kernel; /* After context switch, the child_task will exit kernel properly (since stack are setup). */

    /* Copy the content of kernel stack */
    for (int i = 0; i < KSTACK_SIZE; i++)
        kstack_pool[child_task_id][i] = kstack_pool[current->task_id][i];

    /* compute the relative address between current task kstack and ustack */
    kstack_offset = kstack_pool[child_task_id] - kstack_pool[current->task_id];
    child_task->tss.sp = (uint64_t) ((unsigned long)trapframe + kstack_offset); // the address of trapframe is the address of current sp_el1

    /* Allocate a physical address then copy the content from parent stack to user stack. */
    child_stack = (char *)kmalloc(USER_STACK_SIZE);
    parent_stack = (char *)phys_to_virt(simulate_walk((unsigned long *)phys_to_virt(current->tss.pgd), USER_STACK_ADDR)); // get the parent stack physical address.
    memcpy(child_stack, parent_stack, USER_STACK_SIZE);

    /* Copy the parent pgd table to child pgd table */
    child_pgd = create_empty_page_table();
    parent_pgd = (unsigned long *)phys_to_virt(current->tss.pgd);
    for (i = 0; i < ENTRIES_PER_TABLE; i++)
        child_pgd[i] = parent_pgd[i];
    /* Update child pgd to another user stack */
    remap_pages(child_pgd, USER_STACK_ADDR, USER_STACK_SIZE, virt_to_phys((unsigned long)child_stack), PD_AP_EL0); // user stack is mapped to 0xffffffffb000
    /* Update child pgd */
    child_task->tss.pgd = virt_to_phys((unsigned long)child_pgd);

    child_trapframe = (struct trapframe *)(child_task->tss.sp);
    child_trapframe->x[0] = 0; // setup the return value of child task (fork())
    trapframe->x[0] = child_task_id; // return the child task id to parent process
    return SYSCALL_SUCCESS;
}

/* Modify the current task state and reschedule. */
int sys_exit(struct trapframe *trapframe) {
#ifdef DEBUG_SYSCALL
    printf("[sys_exit] Task %d exit\n", current->task_id);
#endif
    current->state = TASK_STOPPED;
    current->exit_state = trapframe->x[0]; // setup exit state
    num_running_task--;
    schedule(); // if we schedule() here, this function never return
    return SYSCALL_SUCCESS;
}

/* Mailbox call, not sure what to return */
int sys_mbox_call(struct trapframe *trapframe)
{
    unsigned char ch = (unsigned char) trapframe->x[0];
    unsigned int *umbox = (unsigned int *) trapframe->x[1]; // user mbox
    unsigned int size = (unsigned int) umbox[0];

    memcpy((void *) mbox, (void *) umbox, size);
    trapframe->x[0] = mbox_call(ch);
    memcpy((void *) umbox, (void *) mbox, size);

    return SYSCALL_SUCCESS;
}

/* Kill, dummy implement */
int sys_kill(struct trapframe *trapframe)
{
    int pid;
    struct task_struct *task;

    pid = (int) trapframe->x[0];
    if (pid == current->task_id || pid < 0 || pid >= NR_TASKS) {
        printf("The given PID is not valid.\n");
        return SYSCALL_ERROR;
    }

    task = &task_pool[pid];
    if (task->state != TASK_RUNNING) {
        printf("Kill a task that is not running\n");
        return SYSCALL_ERROR;
    }
    task->state = TASK_STOPPED;
    task->exit_state = 1;
    num_running_task--;

    return SYSCALL_SUCCESS;
}

/* register a signal with the pair of signal number and handler function. */
int sys_signal(struct trapframe *trapframe)
{
    int signum;
    void (*handler)(void);

    signum = (int) trapframe->x[0];
    if (signum < 0 || signum >= NR_SIGNALS) {
        printf("The given signal number is not valid.\n");
        return SYSCALL_ERROR;
    }

    handler = (void *) trapframe->x[1];
    current->sighand.action[signum] = handler;
    return SYSCALL_SUCCESS;
}

/* Use signal to kill a process. */
int sys_sigkill(struct trapframe *trapframe)
{
    int pid, signum;

    pid = (int) trapframe->x[0];
    signum = (int) trapframe->x[1];

    if (pid == current->task_id || pid < 0 || pid >= NR_TASKS) {
        printf("The given PID is not valid.\n");
        return SYSCALL_ERROR;
    }

    if (signum < 0 || signum >= NR_SIGNALS) {
        printf("The given signal number is not valid.\n");
        return SYSCALL_ERROR;
    }

    task_pool[pid].pending = signum;

    return SYSCALL_SUCCESS;
}

int sys_sigreturn(struct trapframe *trapframe)
{
#ifdef DEBUG_SYSCALL
    printf("[sys_sigreturn] Task %d return from signal handler\n", current->task_id);
#endif

    /* Free the signal stack. */
    kfree((void *) trapframe->sp);

    /* Restore the previous stack (before signal handling) */
    current->tss.sp = current->tss.sp_backup;
    sig_restore_context(&current->tss);
    return SYSCALL_SUCCESS;
}
