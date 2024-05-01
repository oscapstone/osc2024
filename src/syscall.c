#include "uart.h"
#include "sched.h"
#include "syscall.h"
#include "exec.h"
#include "exception.h"

// #define DEBUG

/* The definition of system call handler function */
syscall_t sys_call_table[SYSCALL_NUM] = {
    sys_get_taskid,
    sys_uart_read,
    sys_uart_write,
    sys_exec,
    sys_fork,
    sys_exit
};

/* As a handler function for svc, setup the return value to trapframe */
int sys_get_taskid(struct trapframe *trapframe)
{
#ifdef DEBUG
    printf("[sys_get_taskid] Task %d get task id\n", current->task_id);
#endif
    trapframe->x[0] = current->task_id;
    return SYSCALL_SUCCESS;
}

/* uart read system call: x0: buffer address, x1: buffer size */
int sys_uart_read(struct trapframe *trapframe)
{
#ifdef DEBUG
    printf("[sys_uart_read] Task %d read\n", current->task_id);
#endif
    /* x0: buffer address, x1: buffer size */
    char *buff_addr = (char *) trapframe->x[0];
    size_t buff_size = (size_t) trapframe->x[1];
    for (int i = 0; i < buff_size; i++)
        buff_addr[i] = uart_async_getc();
    trapframe->x[0] = buff_size; // return the number of bytes read
#ifdef DEBUG
    printf("[sys_uart_read] Task %d read end\n", current->task_id);
#endif
    return SYSCALL_SUCCESS;
}

/* uart write system call: x0: buffer address, x1: buffer size */
int sys_uart_write(struct trapframe *trapframe)
{
#ifdef DEBUG
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
#ifdef DEBUG
    printf("[sys_exec] Task %d exec\n", current->task_id);
#endif
    do_exec((void (*)(void))trapframe->x[0]);
    trapframe->x[0] = 0; // 0 means success
    return SYSCALL_SUCCESS;
}

int sys_fork(struct trapframe *trapframe)
{
#ifdef DEBUG
    printf("[sys_fork] Task %d fork\n", current->task_id);
#endif
    int child_task_id = find_empty_task();
    struct task_struct *child_task = &task_pool[child_task_id];

    /* Copy the content of parent's task_struct and update it based on child task */
    *child_task = *current;
    child_task->task_id = child_task_id;
    child_task->counter = child_task->priority;
    child_task->tss.lr = (uint64_t) &exit_kernel; // current->tss.lr is not up-to-date, so we use exit_kernel

    /* Copy the content of kstack and ustack */
    for (int i = 0; i < KSTACK_SIZE; i++) {
        kstack_pool[child_task_id][i] = kstack_pool[current->task_id][i];
        ustack_pool[child_task_id][i] = ustack_pool[current->task_id][i];
    }

    // compute the relative address between current task kstack and ustack
    int kstack_offset = kstack_pool[child_task_id] - kstack_pool[current->task_id];
    int ustack_offset = ustack_pool[child_task_id] - ustack_pool[current->task_id];

    // sp should be placed according to the relative address (offset)
    char *sp_addr = (char *) trapframe; // the address of trapframe is the address of current sp (el1)
    child_task->tss.sp = (uint64_t) (sp_addr + kstack_offset);

    char *sp_el0_addr = (char *) trapframe->sp; // get the current address of sp_el0 (current task)
    struct trapframe *child_trapframe = (struct trapframe *)(child_task->tss.sp);
    child_trapframe->sp = (uint64_t) (sp_el0_addr + ustack_offset);
    child_trapframe->x[0] = 0; // setup the return value of child task (fork())

    trapframe->x[0] = child_task_id; // return the child task id
    return SYSCALL_SUCCESS;
}

/* Modify the current task state and reschedule. */
int sys_exit(struct trapframe *trapframe) {
#ifdef DEBUG
    printf("[sys_exit] Task %d exit\n", current->task_id);
#endif
    current->state = TASK_STOPPED;
    current->exit_state = trapframe->x[0]; // setup exit state
    num_running_task--;
    // printf("Task %d exit, exit state %d\n", current->task_id, current->exit_state);
    schedule(); // if we schedule() here, this function never return
    return SYSCALL_SUCCESS;
}
