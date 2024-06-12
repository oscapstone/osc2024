#include "uart.h"
#include "allocator.h"
#include "timer.h"
#include "cpio.h"
#include "exception.h"
#include "utils.h"
#include "mm.h"
#include "schedule.h"

task_list run_queue;
int task_id = 0;

void sched_init()
{
    for (int i = 0; i < 10; i++)
        run_queue.head[i] = NULL;

    // create idle task
    task_create(zombie_reaper, 0);
    asm volatile("msr tpidr_el1, %0" : : "r"(run_queue.head[0]));
}

void run_queue_push(task_struct *new_task, int priority)
{
    if (run_queue.head[priority] == NULL) // the head is empty
        run_queue.head[priority] = new_task;
    else
    {
        new_task->next = run_queue.head[priority];
        run_queue.head[priority]->prev = new_task;
        run_queue.head[priority] = new_task;
    }
}

void run_queue_remove(task_struct *remove_task, int priority)
{
    if (run_queue.head[priority] == remove_task) // the remove task is the front of the list
    {
        run_queue.head[priority] = run_queue.head[priority]->next;
        if (run_queue.head[priority] != NULL)
            run_queue.head[priority]->prev = NULL;
    }
    else
    {
        if (remove_task->next != NULL)
            remove_task->next->prev = remove_task->prev;

        if (remove_task->prev != NULL)
            remove_task->prev->next = remove_task->next;
    }
}

task_struct *task_create(void (*start_routine)(void), int priority)
{
    disable_interrupt();

    // create new task and initialize it.
    task_struct *new_task = kmalloc(sizeof(task_struct));
    new_task->id = task_id++;
    new_task->priority = priority;

    new_task->mm_struct = (struct mm_struct *)kmalloc(sizeof(struct mm_struct));
    new_task->mm_struct->mmap = NULL;
    new_task->mm_struct->pgd = NULL;
    new_task->kstack = (void *)((char *)kmalloc(4096 * 5) + 4096 * 4); // malloc space for kernel stack
    new_task->ustack = (void *)((char *)kmalloc(4096 * 5) + 4096 * 4); // malloc space for user stack

    new_task->cpu_context.sp = (unsigned long long)(new_task->kstack); // set stack pointer
    new_task->cpu_context.fp = (unsigned long long)(new_task->kstack); // set frame pointer
    new_task->cpu_context.lr = (unsigned long long)start_routine;      // set linker register

    new_task->state = RUNNING;
    new_task->need_sched = 0;
    new_task->received_signal = NOSIG;

    new_task->pwd = rootfs->root;
    for (int i = 0; i < NR_OPEN_DEFAULT; i++)
        new_task->fd_array[i] = NULL;
    new_task->fd_array[0] = new_task->fd_array[1] = new_task->fd_array[2] = vfs_open("/dev/uart", 0);

    for (int i = 0; i < SIG_NUM; i++)
    {
        new_task->is_default_signal_handler[i] = 1;
        new_task->signal_handler[i] = NULL;
    }
    new_task->signal_handler[9] = default_SIGKILL_handler;

    new_task->prev = NULL;
    new_task->next = NULL;

    run_queue_push(new_task, priority);

    return new_task;
}

void task_exit()
{
    task_struct *cur_task = get_current_task();
    cur_task->state = EXIT; // set the task state to EXIT

    schedule(); // yield the CPU
}

void zombie_reaper()
{
    while (1)
    {
        kill_zombies();
        schedule();
    }
}

void kill_zombies()
{
    for (int i = 0; i < 10; i++)
    {
        task_struct *cur = run_queue.head[i];
        while (cur != NULL)
        {
            if (cur->state == EXIT)
            {
                task_struct *del = cur;
                cur = cur->next;

                disable_interrupt();

                run_queue_remove(del, i); // remove zombie task

                for (struct vm_area_struct *vma = del->mm_struct->mmap; vma != NULL;) // clean all VMA
                {
                    struct vm_area_struct *temp = vma;
                    vma = vma->vm_next;
                    kfree(temp);
                }
                page_reclaim(del->mm_struct->pgd); // reclaim all page entry

                kfree(del->mm_struct);
                kfree((char *)del->kstack - 4096 * 4); // free the stack space
                
                kfree(del);

                enable_interrupt();
            }
            else
                cur = cur->next;
        }
    }
}

void schedule()
{
    task_struct *cur = get_current_task()->next;
    while (cur != NULL && (cur->state == IDLE || cur->state == EXIT)) // fine a process that it's state is running
        cur = cur->next;

    if (cur == NULL)
        cur = run_queue.head[0];

    context_switch(cur);
}

void context_switch(struct task_struct *next)
{
    struct task_struct *prev = get_current_task();
    if (prev != next)
    {
        switch_mm_irqs_off(next->mm_struct->pgd); // change the page table
        switch_to(prev, next);
    }
}

void check_need_schedule()
{
    struct task_struct *cur = get_current_task();
    if (cur->need_sched == 1) // if the need_sched flag of thecurrent task is set, then call schedle()
    {
        cur->need_sched = 0;
        schedule();
    }
}

void check_signal(struct ucontext *sigframe) // the sigframe is the user context
{
    struct task_struct *cur = get_current_task();
    int SIGNAL = cur->received_signal;

    if (SIGNAL != NOSIG)
    {
        if (cur->is_default_signal_handler[SIGNAL] == 1) // the signal handler is default, and run in kernel mode.
        {
            cur->received_signal = NOSIG;
            cur->signal_handler[SIGNAL]();
        }
        else
        {
            cur->received_signal = NOSIG;
            do_signal(sigframe, cur->signal_handler[SIGNAL]); // the signal handler is user-defined, so run in user mode.
        }
    }
}

void do_exec(const char *name, char *const argv[])
{
    struct file *file = vfs_open(name, 0);
    FILE *initramfs_internal = (struct FILE *)file->f_dentry->d_inode->internal;

    char *content = initramfs_internal->file_content;
    int size = given_size_hex_atoi(initramfs_internal->file_header->c_filesize, 8);
    vfs_close(file);

    task_struct *cur = get_current_task();
    cur->priority = 1;

    disable_interrupt();
    
    char *target = kmalloc(size);
    char *copy = target;

    for (int i = 0; i < SIG_NUM; i++) // inititalize signal_handler
    {
        cur->is_default_signal_handler[i] = 1;
        cur->signal_handler[i] = NULL;
    }
    cur->signal_handler[9] = default_SIGKILL_handler;

    init_mm_struct(cur->mm_struct);
    // map code
    mappages(cur->mm_struct, CODE, 0, (unsigned long long)target - VA_START, size, PROT_READ | PROT_EXEC, MAP_ANONYMOUS);
    // map ustack
    mappages(cur->mm_struct, STACK, 0xffffffffb000, (unsigned long long)(cur->ustack) - 4096 * 4 - VA_START, 4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS);
    mappages(cur->mm_struct, STACK, 0xffffffffc000, (unsigned long long)(cur->ustack) - 4096 * 3 - VA_START, 4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS);
    mappages(cur->mm_struct, STACK, 0xffffffffd000, (unsigned long long)(cur->ustack) - 4096 * 2 - VA_START, 4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS);
    mappages(cur->mm_struct, STACK, 0xffffffffe000, (unsigned long long)(cur->ustack) - 4096 * 1 - VA_START, 4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS);
    mappages(cur->mm_struct, STACK, 0xfffffffff000, (unsigned long long)(cur->ustack) - VA_START, 4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS);

    while (size--) // move the file content to memory
        *copy++ = *content++;

    switch_mm_irqs_off(cur->mm_struct->pgd);

    enable_interrupt();
    
    // set sp_el0 to user stack, sp_el1 to kernels stack, elr_el1 to the file content
    asm volatile(
        "msr sp_el0, %0\n"
        "msr elr_el1, %1\n"
        "mov x10, 0\n"
        "msr spsr_el1, x10\n"
        "mov sp, %2\n"
        "eret\n"
        :
        : "r"(0xfffffffff000), "r"(0x0), "r"(cur->kstack));
}