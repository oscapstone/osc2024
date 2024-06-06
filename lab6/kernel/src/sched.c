#include "sched.h"
#include "uart1.h"
#include "exception.h"
#include "memory.h"
#include "timer.h"
#include "shell.h"
#include "signal.h"
#include "stdio.h"
#include "mmu.h"
#include "string.h"

list_head_t *run_queue;
execfile c_execfile;

thread_t threads[PIDMAX + 1];
thread_t *curr_thread;

int pid_history = 0;
int timer_sched_flag = 0;
int shell_flag = 0;

void thread_sched_init()
{
    lock();
    // init thread freelist and run_queue
    run_queue = kmalloc(sizeof(list_head_t));
    INIT_LIST_HEAD(run_queue);

    // init pids
    for (int i = 0; i <= PIDMAX; i++)
    {
        threads[i].isused = 0;
        threads[i].pid = i;
        threads[i].iszombie = 0;
    }

    thread_t *idlethread = thread_create(start_shell);
    curr_thread = idlethread;
    asm volatile("msr tpidr_el1, %0" ::"r"(&curr_thread->context)); // Don't let thread structure NULL as we enable the functionality

    // thread_create(idle);
    unlock();
}

thread_t *thread_create(void *start)
{
    lock();
    thread_t *r;
    // find usable PID, don't use the previous one
    if (pid_history > PIDMAX)
    {
        unlock();
        return 0;
    }

    if (!threads[pid_history].isused)
    {
        r = &threads[pid_history];
        pid_history += 1;
    }
    else
    {
        unlock();
        return 0;
    }

    INIT_LIST_HEAD(&r->vma_list);
    r->iszombie = 0;
    r->isused = 1;
    r->stack_alloced_ptr = kmalloc(USTACK_SIZE);
    r->kernel_stack_alloced_ptr = kmalloc(KSTACK_SIZE);
    r->context.lr = (unsigned long long)start;
    r->context.sp = (unsigned long long)r->kernel_stack_alloced_ptr + KSTACK_SIZE - STACK_BASE_OFFSET;
    r->context.fp = r->context.sp; // frame pointer for local variable, which is also in stack.

    // new <---------------------------------------------------------------------------------------------------------------
    // r->data = kmalloc(c_execfile.filesize);
    // r->datasize = c_execfile.filesize;
    r->context.pgd = kmalloc(0x1000);
    // uart_sendlinek("r->context.pgd: %x\n", r->context.pgd);
    memset(r->context.pgd, 0, 0x1000);
    r->context.pgd = (void *)KERNEL_VIRT_TO_PHYS(r->context.pgd);
    // new <---------------------------------------------------------------------------------------------------------------

    r->signal_is_checking = 0;
    // initial all signal handler with signal_default_handler (kill thread)

    // uart_sendlinek("signal_default_handler : 0x%x\n", signal_default_handler);
    for (int i = 0; i < SIGNAL_MAX; i++)
    {
        r->signal_handler[i] = signal_default_handler;
        r->sigcount[i] = 0;
    }

    list_add_tail(&r->listhead, run_queue);
    unlock();
    return r;
}

void schedule()
{
    lock();
    // uart_sendlinek("Run queue size :%d \n", list_size(run_queue));
    do
    {
        // uart_sendlinek("Run queue size :%d \n", list_size(run_queue));
        curr_thread = (thread_t *)curr_thread->listhead.next;
    } while (list_is_head(&curr_thread->listhead, run_queue)); // find a runnable thread
    unlock();
    // uart_sendlinek("curr_thread :%d \n",curr_thread->pid);
    // uart_sendlinek("curr_thread->context.lr :0x%x \n",curr_thread->context.lr);
    switch_to(get_current(), &curr_thread->context);
}

void idle()
{
    // uart_sendlinek("This is idle\n");
    while (shell_flag > 0)
    // while (list_size(run_queue) > 1)
    {
        // uart_sendlinek("This is idle\n");
        kill_zombies(); // reclaim threads marked as DEAD
        schedule();     // switch to next thread in run queue
        // delay(10000);
    }
    kill_zombies();
}

void thread_exit()
{
    // thread cannot deallocate the stack while still using it, wait for someone to recycle it.
    // In this lab, idle thread handles this task, instead of parent thread.
    lock();
    curr_thread->iszombie = 1;
    shell_flag--;
    unlock();
    // delay(10000);
    // schedule();
}

void kill_zombies()
{
    lock();
    list_head_t *curr;
    thread_t *t;
    list_for_each(curr, run_queue)
    {
        t = (thread_t *)curr;
        if (t->iszombie)
        {
            list_del_entry(curr);
            mmu_free_page_tables(t->context.pgd, 0);
            mmu_del_vma(t);
            kfree(t->kernel_stack_alloced_ptr);
            kfree((void*)PHYS_TO_KERNEL_VIRT(t->context.pgd));
            t->iszombie = 0;
            t->isused = 0;
        }
    }
    unlock();
}

void foo()
{
    // Lab5 Basic 1 Test function
    for (int i = 0; i < 10; ++i)
    {
        uart_sendlinek("Thread id: %d %d\n", curr_thread->pid, i);
        int r = 1000000;
        while (r--)
        {
            asm volatile("nop");
        }
        schedule();
    }
    // uart_sendlinek("exit\n");
    thread_exit();
}

int exec_thread()
{
    lock();
    shell_flag++;
    thread_t *t = thread_create(exec_proc);
    curr_thread = t;
    // uart_sendlinek("timer_sched_flag : %d\n", timer_sched_flag);
    if (!timer_sched_flag)
    {
        timer_sched_flag = 1;
        add_timer(schedule_timer, 1, "", setSecond); // start scheduler ---------------------------------------------------------
    }

    unlock();
    schedule();

    return 0;
}

void exec_proc()
{
    lock();
    char *data = c_execfile.data;
    unsigned int filesize = c_execfile.filesize;
    thread_t *t = curr_thread;
    // uart_sendlinek("filesize : %d\n", filesize);
    t->data = kmalloc(filesize);
    t->datasize = filesize;
    // t->context.lr = (unsigned long)t->data; // set return address to program if function call completes

    mmu_add_vma(t, USER_DATA_BASE, t->datasize, (size_t)KERNEL_VIRT_TO_PHYS(t->data), 0b111, 1, USER_DATA);
    mmu_add_vma(t, USER_STACK_BASE - USTACK_SIZE, USTACK_SIZE, (size_t)KERNEL_VIRT_TO_PHYS(t->stack_alloced_ptr), 0b111, 1, USER_STACK);
    mmu_add_vma(t, PERIPHERAL_START, PERIPHERAL_END - PERIPHERAL_START, PERIPHERAL_START, 0b011, 0, PERIPHERAL);
    mmu_add_vma(t, USER_SIGNAL_WRAPPER_VA, 0x1000, ALIGN_DOWN((size_t)KERNEL_VIRT_TO_PHYS(signal_handler_wrapper), PAGESIZE), 0b101, 0, USER_SIGNAL_WRAPPER);
    mmu_add_vma(t, USER_EXEC_WRAPPER_VA, 0x2000, ALIGN_DOWN((size_t)KERNEL_VIRT_TO_PHYS(exec_wrapper), PAGESIZE), 0b101, 0, USER_EXEC_WRAPPER);

    // t->context.pgd = KERNEL_VIRT_TO_PHYS(t->context.pgd);
    t->context.sp = USER_STACK_BASE - STACK_BASE_OFFSET;
    t->context.fp = USER_STACK_BASE - STACK_BASE_OFFSET;
    t->context.lr = USER_DATA_BASE;

    // copy file into data
    for (int i = 0; i < filesize; i++)
    {
        t->data[i] = data[i];
    }
    // dump_vma();
    // eret to exception level 0
    // lock();

    // asm("dsb ish\n\t" // ensure write has completed
    //     "msr ttbr0_el1, %0\n\t"
    //     "tlbi vmalle1is\n\t" // invalidate all TLB entries
    //     "dsb ish\n\t"        // ensure completion of TLB invalidatation
    //     "isb\n\t"            // clear pipeline"
    //     ::"r"(t->context.pgd));

    asm("msr tpidr_el1, %0\n\t" // Hold the "kernel(el1)" thread structure information
        "msr elr_el1, %1\n\t"   // When el0 -> el1, store return address for el1 -> el0
        "msr spsr_el1, xzr\n\t" // EL1h (SPSel = 1) with interrupt disabled
        "msr sp_el0, %2\n\t"    // el0 stack pointer for el1 process
        "mov sp, %3\n\t"        // sp is reference for the same el process. For example, el2 cannot use sp_el2, it has to use sp to find its own stack.
        "mov x0, %4\n\t" ::"r"(&t->context),
        "r"(exec_wrapper), "r"(t->context.sp), "r"(t->kernel_stack_alloced_ptr + KSTACK_SIZE - STACK_BASE_OFFSET), "r"(t->context.lr));

    //unlock();
    asm("eret\n\t");
}

void exec_wrapper()
{
    // unlock
    // uart_sendlinek("exec_handler : %x\n", exec_addr);
    // uart_sendlinek("curr_thread : %x\n", curr_thread);
    // uart_sendlinek(curr_thread->context.lr));
    asm("mov x8,514\n\t"
        "svc 0\n\t");

    asm("blr x0\n\t"
        "mov x8,50\n\t"
        "svc 0\n\t");
}

void schedule_timer(char *notuse)
{
    unsigned long long cntfrq_el0;
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t" : "=r"(cntfrq_el0));
    add_timer(schedule_timer, cntfrq_el0 >> 5, "", setTick);
    // schedule();
    //  32 * default timer -> trigger next schedule timer
}
