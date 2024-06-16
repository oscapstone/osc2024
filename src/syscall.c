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
#include "vfs.h"

#define DEBUG_FS_SYSCALL

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
    sys_sigreturn,
    sys_open,
    sys_close,
    sys_write,
    sys_read,
    sys_mkdir,
    sys_mount,
    sys_chdir,
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
    unsigned int child_task_id, kstack_offset;
    struct task_struct *child_task;
    struct trapframe *child_trapframe;
    unsigned long *child_pgd;
    char *child_stack, *parent_stack;

    child_task_id = find_empty_task();
    child_task = &task_pool[child_task_id];

    /* Copy the content of parent's task_struct and update it based on child task */
    *child_task = *current;
    child_task->task_id = child_task_id;
    child_task->counter = child_task->priority;
    child_task->tss.lr = (uint64_t) &exit_kernel; /* After context switch, the child_task will exit kernel properly (since stack are setup). */

    /* Copy the content of kernel stack */
    memcpy(kstack_pool[child_task_id], kstack_pool[current->task_id], KSTACK_SIZE);

    /* compute the relative address between current task kstack and ustack */
    kstack_offset = kstack_pool[child_task_id] - kstack_pool[current->task_id];
    child_task->tss.sp = (uint64_t) ((unsigned long)trapframe + kstack_offset); // the address of trapframe is the address of current sp_el1

    /* Copy the parent pgd table to child pgd table */
    child_pgd = create_empty_page_table();

    /* Allocate a physical address then copy the content from parent stack to user stack. */
    child_stack = (char *)kmalloc(USER_STACK_SIZE);
    parent_stack = (char *)phys_to_virt(simulate_walk((unsigned long *)phys_to_virt(current->tss.pgd), USER_STACK_ADDR)); // get the parent stack physical address.
    memcpy(child_stack, parent_stack, USER_STACK_SIZE);

    /* Copy the content of parent process's user program */
    child_task->mm.prog_addr = (unsigned long) kmalloc(current->mm.prog_sz);
    child_task->mm.prog_sz = current->mm.prog_sz;
    memcpy((void *) child_task->mm.prog_addr, (void *) current->mm.prog_addr, current->mm.prog_sz);

    /* Update child pgd */
    map_pages(child_pgd, USER_STACK_ADDR, USER_STACK_SIZE, virt_to_phys((unsigned long)child_stack), PD_AP_EL0); // user stack is mapped to 0xffffffffb000
    map_pages(child_pgd, USER_PROG_START, current->mm.prog_sz, virt_to_phys(child_task->mm.prog_addr), PD_AP_EL0); // user program is mapped to 0x0
    map_pages(child_pgd, PERIPHERAL_BASE, PERIPHERAL_SIZE, PERIPHERAL_BASE, PD_AP_EL0); // map peripheral to the same address

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


int sys_open(struct trapframe *trapframe)
{
    char *path = (char *) trapframe->x[0];
    int flags = (int) trapframe->x[1], i;
    char absolute_path[MAX_PATH_LEN];

#ifdef DEBUG_FS_SYSCALL
    printf("[sys_open] Task %d open %s\n", current->task_id, path);
#endif

    strcpy(absolute_path, path);
    /* Update absolute path from current working directory */
    get_absolute_path(absolute_path, current->current_dir);
    for (i = 0; i < FD_TABLE_SIZE; i++) {
        if (!current->fd_table[i]) { // if the file descriptor is empty
            if (vfs_open(absolute_path, flags, &current->fd_table[i])) {
                trapframe->x[0] = i; // return the file descriptor
                return SYSCALL_SUCCESS;
            } else {
                printf("[sys_open] vfs_open function failed.\n");
                trapframe->x[0] = -1;
                return SYSCALL_ERROR;
            }
        }
    }
    printf("[sys_open] No available file descriptor.\n");
    trapframe->x[0] = -1;
    return SYSCALL_ERROR;
}

int sys_close(struct trapframe *trapframe)
{
    int fd = (int) trapframe->x[0];

#ifdef DEBUG_FS_SYSCALL
    printf("[sys_close] Task %d close fd %d\n", current->task_id, fd);
#endif
    
    if (current->fd_table[fd]) {
        if (vfs_close(current->fd_table[fd])) {
            current->fd_table[fd] = NULL;
            trapframe->x[0] = 0;
            return SYSCALL_SUCCESS;
        } else {
            printf("[sys_close] vfs_close function failed.\n");
            trapframe->x[0] = -1;
            return SYSCALL_ERROR;
        }
    }
    printf("[sys_close] The file descriptor is not valid.\n");
    trapframe->x[0] = -1;
    return SYSCALL_ERROR;
}

int sys_write(struct trapframe *trapframe)
{
    int fd = (int) trapframe->x[0];
    char *buf = (char *) trapframe->x[1];
    size_t len = (size_t) trapframe->x[2];

#ifdef DEBUG_FS_SYSCALL
    printf("[sys_write] fd: %d, buf: %x, len: %d\n", fd, buf, len);
#endif

    if (current->fd_table[fd]) {
        trapframe->x[0] = vfs_write(current->fd_table[fd], buf, len);
        return SYSCALL_SUCCESS;
    }
    printf("[sys_write] The file descriptor is not valid.\n");
    trapframe->x[0] = -1;
    return SYSCALL_ERROR;
}

int sys_read(struct trapframe *trapframe)
{
    int fd = (int)trapframe->x[0];
    char *buf = (char *)trapframe->x[1];
    size_t len = (size_t)trapframe->x[2];

#ifdef DEBUG_FS_SYSCALL
    printf("[sys_read] fd: %d, buf: %x, len: %d\n", fd, buf, len);
#endif

    if (current->fd_table[fd]) {
        trapframe->x[0] = vfs_read(current->fd_table[fd], buf, len);
        return SYSCALL_SUCCESS;
    }
    printf("[sys_read] The file descriptor is not valid.\n");
    trapframe->x[0] = -1;
    return SYSCALL_ERROR;
}

int sys_mkdir(struct trapframe *trapframe)
{
    char *path = (char *)trapframe->x[0];

#ifdef DEBUG_FS_SYSCALL
    printf("[sys_mkdir] Task %d mkdir %s\n", current->task_id, path);
#endif
    get_absolute_path(path, current->current_dir);
    trapframe->x[0] = vfs_mkdir(path);
    return SYSCALL_SUCCESS;
}

int sys_mount(struct trapframe *trapframe)
{
    const char *target = (char *)trapframe->x[1];
    const char *file_system = (char *)trapframe->x[2];
    char absolute_path[MAX_PATH_LEN];

#ifdef DEBUG_FS_SYSCALL
    printf("[sys_mount] Task %d mount file system %s at path %s\n", current->task_id, file_system, target);
#endif

    strcpy(absolute_path, target);
    get_absolute_path(absolute_path, current->current_dir);

    trapframe->x[0] = vfs_mount(absolute_path, file_system);
    return SYSCALL_SUCCESS;
}

int sys_chdir(struct trapframe *trapframe)
{
    const char *path = (char *)trapframe->x[0];
    char absolute_path[MAX_PATH_LEN];
    struct vnode *dir;

#ifdef DEBUG_FS_SYSCALL
    printf("[sys_chdir] Task %d change directory to %s\n", current->task_id, path);
#endif

    strcpy(absolute_path, path);
    get_absolute_path(absolute_path, current->current_dir);
    if (!vfs_lookup(absolute_path, &dir)) {
        printf("[sys_chdir] change to a empty directory %s\n", absolute_path);
        trapframe->x[0] = -1;
        return SYSCALL_ERROR;
    }
    strcpy(current->current_dir, absolute_path);
    trapframe->x[0] = 0;
    return SYSCALL_SUCCESS;
}