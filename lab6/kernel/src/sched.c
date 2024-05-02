#include "sched.h"
#include "exception.h"
#include "memory.h"
#include "timer.h"
#include "uart1.h"
#include "signal.h"
#include "mmu.h"
#include "string.h"
#include "shell.h"

thread_t *curr_thread;
list_head_t *run_queue;
list_head_t *wait_queue;
thread_t threads[PIDMAX + 1];
int finish_init_thread_sched = 0;

void init_thread_sched()
{
    lock();
    run_queue = kmalloc(sizeof(list_head_t));
    wait_queue = kmalloc(sizeof(list_head_t));
    INIT_LIST_HEAD(run_queue);
    INIT_LIST_HEAD(wait_queue);

    //init pids
    for (int i = 0; i <= PIDMAX; i++)
    {
        threads[i].isused = 0;
        threads[i].pid = i;
        threads[i].iszombie = 0;
    }

    curr_thread = thread_create(idle,0x1000, IDLE_PRIORITY);
    thread_create(cli_cmd, 0x1000, SHELL_PRIORITY);

    asm volatile("msr tpidr_el1, %0" ::"r"(curr_thread + sizeof(list_head_t)));
    finish_init_thread_sched = 1;
    // add_timer(schedule_timer, 1, "", 0); // start scheduler
    unlock();
}
void idle(){
    while(1)
    {
        kill_zombies();   //reclaim threads marked as DEAD
        el1_interrupt_enable();
        schedule();
    }
}
void add_task_to_runqueue(thread_t *t)
{    
    lock();
    list_head_t *index_threrad;
    list_for_each(index_threrad, run_queue)
    {
        if (((thread_t*)index_threrad)->priority > curr_thread->priority)
        {
            list_add(&t->listhead, index_threrad->prev);
            break;
        }
    }

    if (list_is_head(index_threrad, run_queue))
    {
        list_add_tail(&t->listhead, run_queue);
    }
    unlock();
}
void schedule(){
    // lock();
    // uart_sendline("schedule 11\r\n");
    list_del_entry(&curr_thread->listhead);
    add_task_to_runqueue(curr_thread);
    curr_thread = (thread_t *)run_queue->next;
    // uart_sendline("curr_thread->pid: %d, is_zombie: %d\r\n", curr_thread->pid, curr_thread->iszombie);
    while(curr_thread->iszombie)
    {
        curr_thread = (thread_t *)curr_thread->listhead.next;
    }
    // uart_sendline("curr_thread->pid: %d, is_zombie: %d\r\n", curr_thread->pid, curr_thread->iszombie);
    switch_to(get_current(), &curr_thread->context);
    // unlock();
}

void kill_zombies(){
    lock();
    list_head_t *curr;
    thread_t *t;
    list_for_each(curr,run_queue)
    {
        t = (thread_t *)curr;
        if (t->iszombie)
        {
            list_del_entry(curr);
            mmu_free_page_tables(t->context.pgd,0);
            mmu_del_vma(t);
            kfree(t->kernel_stack_alloced_ptr);
            kfree(PHYS_TO_VIRT(t->context.pgd));
            t->iszombie = 0;
            t->isused   = 0;
        }
    }
    unlock();
}

int thread_exec(char *data, unsigned int filesize)
{
    thread_t *t_thread = thread_create(run_user_code, filesize, 100);
    lock();
    // copy file into data
    for (int i = 0; i < filesize;i++)
    {
        t_thread->data[i] = data[i];
    }

    curr_thread = t_thread;
    
    // eret to exception level 0
    unlock();
    switch_to(get_current(), &t_thread->context);


    return 0;
}
void run_user_code()
{
    lock();
    // add vma                             Virtual Address,                              Size,                             Physical Address,            rwx, is_alloced
    mmu_add_vma(curr_thread,              USER_KERNEL_BASE,             curr_thread->datasize,                (size_t)VIRT_TO_PHYS(curr_thread->data), 0b111, 1);
    mmu_add_vma(curr_thread, USER_STACK_BASE - USTACK_SIZE,                       USTACK_SIZE,   (size_t)VIRT_TO_PHYS(curr_thread->stack_alloced_ptr), 0b111, 1);
    mmu_add_vma(curr_thread,              PERIPHERAL_START, PERIPHERAL_END - PERIPHERAL_START,                                       PERIPHERAL_START, 0b011, 0);
    mmu_add_vma(curr_thread,        USER_SIGNAL_WRAPPER_VA,                            0x2000,           (size_t)VIRT_TO_PHYS(signal_handler_wrapper), 0b101, 0);
    curr_thread->context.pgd = VIRT_TO_PHYS(curr_thread->context.pgd);
    curr_thread->context.sp = USER_STACK_BASE;
    curr_thread->context.fp = USER_STACK_BASE;

    unlock();

    asm("msr tpidr_el1, %0\n\t"
        "msr elr_el1, %1\n\t"
        "msr spsr_el1, xzr\n\t" // enable interrupt in EL0. You can do it by setting spsr_el1 to 0 before returning to EL0.
        "msr sp_el0, %2\n\t"
        "mov sp, %3\n\t"
        "dsb ish\n\t"        // ensure write has completed
        "msr ttbr0_el1, %4\n\t"
        "tlbi vmalle1is\n\t" // invalidate all TLB entries
        "dsb ish\n\t"        // ensure completion of TLB invalidatation
        "isb\n\t"            // clear pipeline"
        "eret\n\t" 
        :
        :   "r"(&curr_thread->context),
            "r"(USER_KERNEL_BASE),       // the start address(0x0000) for user code
            "r"(curr_thread->context.sp), 
            "r"(curr_thread->kernel_stack_alloced_ptr + KSTACK_SIZE),
            "r"(curr_thread->context.pgd)
    );
}

//malloc a kstack and a userstack
thread_t *thread_create(void *start, unsigned int filesize, int priority)
{
    // uart_sendline("thread_create\r\n");
    lock();
    thread_t *r;
    for (int i = 0; i <= PIDMAX; i++)
    {
        if (!threads[i].isused)
        {
            r = &threads[i];
            break;
        }
    }
    INIT_LIST_HEAD(&r->vma_list);
    r->priority = priority;
    r->iszombie = 0;
    r->isused = 1;
    r->context.lr = (unsigned long long)start;
    r->stack_alloced_ptr = kmalloc(USTACK_SIZE);
    r->kernel_stack_alloced_ptr = kmalloc(KSTACK_SIZE);
    r->signal_is_checking = 0;
    r->data = kmalloc(filesize);
    r->datasize = filesize;
    r->context.sp = (unsigned long long)r->kernel_stack_alloced_ptr + KSTACK_SIZE;
    r->context.fp = r->context.sp;
    r->context.pgd = kmalloc(0x1000);
    memset(r->context.pgd, 0, 0x1000);
    //initial signal handler with signal_default_handler (kill thread)
    for (int i = 0; i < SIGNAL_MAX; i++)
    {
        r->signal_handler[i] = signal_default_handler;
        r->sigcount[i] = 0;
    }
    add_task_to_runqueue(r);

    unlock();
    return r;
}

void thread_exit(){
    lock();
    uart_sendline("Thread %d exit\n", curr_thread->pid);
    curr_thread->iszombie = 1;
    unlock();
    schedule();
}

void schedule_timer(char* notuse){
    unsigned long long cntfrq_el0;
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t": "=r"(cntfrq_el0)); //tick frequency
    add_timer(schedule_timer, cntfrq_el0 >> 5, "", 1);
}
