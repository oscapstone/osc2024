#include "sched.h"
#include "irq.h"
#include "malloc.h"
#include "timer.h"
#include "uart.h"
#include "mmu.h"
#include "current.h"
#include "signal.h"
#include "string.h"

thread_t *curr_thread;
list_head_t *run_queue;
list_head_t *wait_queue;
thread_t threads[PIDMAX + 1];

void init_thread_sched()
{
    lock();
    // malloc run & wait queue
    run_queue = malloc(sizeof(list_head_t));
    wait_queue = malloc(sizeof(list_head_t));
    INIT_LIST_HEAD(run_queue);
    INIT_LIST_HEAD(wait_queue);

    // init pids
    // 類似 thread pool
    for (int i = 0; i <= PIDMAX; i++)
    {
        threads[i].pid = i;
        threads[i].status = NEW;
    }
    // set current tmp thread
    thread_t *tmp = malloc(sizeof(thread_t));
    // set tpidr_el1
    set_current_ctx(&tmp->context);
    curr_thread = tmp;

    // create idle
    thread_create(idle, 0x1000);
    unlock();
}

void idle() {
    while (1) {   
        // reclaim threads marked as DEAD
        kill_zombies(); 
        // switch to next thread in run_queue
        schedule();     
    }
}

void schedule() {
    lock();
    do {
        curr_thread = (thread_t *)curr_thread->listhead.next;
    } while (list_is_head(&curr_thread->listhead, run_queue) || curr_thread->status == DEAD);
    // switch to next thread
   
    unlock();
    switch_to(current_ctx, &curr_thread->context);
}

void kill_zombies()
{
    lock();
    list_head_t *tmp;
    list_for_each(tmp, run_queue)
    {
        thread_t *tmp_thread = (thread_t *)tmp;
        if (tmp_thread->status == DEAD)
        {
            list_del(tmp);
            // free(tmp_thread->user_sp);   // free stack
            free(tmp_thread->kernel_sp); // free stack
            free_page_tables(tmp_thread->context.ttbr0_el1, 0);
            //   free(tmp_thread->data); // free data

            list_head_t *vma_tmp = tmp_thread->vma_list.next;
            while (vma_tmp != &tmp_thread->vma_list)
            {   
                // if is_alloced free malloced vma
                if (((vm_area_struct_t *)vma_tmp)->is_alloced)
                    free((void *)PHYS_TO_VIRT(((vm_area_struct_t *)vma_tmp)->phys_addr));

                list_head_t *next_vma = vma_tmp->next;
                free(vma_tmp);
                vma_tmp = next_vma;
            }

            free(PHYS_TO_VIRT(tmp_thread->context.ttbr0_el1)); // free PGD

            tmp_thread->status = NEW;
        }
    }
    unlock();
}

int exec_thread(char *data, unsigned int filesize)
{
    thread_t *t = thread_create(data, filesize);

    add_vma(t, 0x3C000000L, 0x3000000L, 0x3C000000L, 3, 0);                                                     // device
    add_vma(t, 0xffffffffb000, 0x4000, (size_t)VIRT_TO_PHYS(t->user_sp), 7, 1);                                 // stack
    add_vma(t, 0x0, filesize, (size_t)VIRT_TO_PHYS(t->data), 7, 1);                                             // text
    add_vma(t, USER_SIG_WRAPPER_VIRT_ADDR_ALIGNED, 0x2000, (size_t)VIRT_TO_PHYS(signal_handler_wrapper), 5, 0); // for signal wrapper

    t->context.ttbr0_el1 = VIRT_TO_PHYS(t->context.ttbr0_el1);
    t->context.sp = 0xfffffffff000;
    t->context.fp = 0xfffffffff000;
    t->context.lr = 0L;

    memcpy(t->data, data, filesize);

    // disable echo when going to userspace
    curr_thread = t;
    // different timer
    add_timer(schedule_timer, "", 1, 0);
    // eret to exception level 0
    __asm__ __volatile__("msr tpidr_el1, %0\n\t"
                         "msr elr_el1, %1\n\t"
                         "msr spsr_el1, xzr\n\t" // enable interrupt in EL0. You can do it by setting spsr_el1 to 0 before returning to EL0.
                         "msr sp_el0, %2\n\t"
                         "mov sp, %3\n\t"
                         "mov fp, sp\n\t"
                         "dsb ish\n\t" // ensure write has completed
                         "msr ttbr0_el1, %4\n\t"
                         // need memory barriers 
                         // gurantee previos instructions are finished
                         // TLB invalidation -> old value are staled
                         "tlbi vmalle1is\n\t" // invalidate all TLB entries
                         "dsb ish\n\t"        // ensure completion of TLB invalidatation
                         "isb\n\t"            // clear pipeline"
                         "eret\n\t" ::"r"(&t->context),
                         "r"(t->context.lr), "r"(t->context.sp), "r"(t->kernel_sp + KSTACK_SIZE), "r"(t->context.ttbr0_el1));

    return 0;
}

thread_t *thread_create(void *start, unsigned int filesize)
{
    lock();

    thread_t *r;
    for (int i = 0; i <= PIDMAX; i++)
    {
        if (threads[i].status == NEW)
        {
            r = &threads[i];
            break;
        }
    }
    INIT_LIST_HEAD(&r->vma_list);
    r->status = RUNNING;
    r->context.lr = (unsigned long long)start;
    r->user_sp = malloc(USTACK_SIZE);
    r->kernel_sp = malloc(KSTACK_SIZE);
    r->signal_is_checking = 0;
    r->data = malloc(filesize);
    r->datasize = filesize;
    r->context.sp = (unsigned long long)r->kernel_sp + KSTACK_SIZE;
    r->context.fp = r->context.sp;
    // create one PGD (page table for this process)
    // set PGD to 0
    // context.ttbr0_el1 point to this PGD
    r->context.ttbr0_el1 = malloc(0x1000);
    memset(r->context.ttbr0_el1, 0, 0x1000);

    // initial signal handler with signal_default_handler (kill thread)
    for (int i = 0; i < SIGNAL_MAX; i++)
    {
        r->signal_handler[i] = signal_default_handler;
        r->sigcount[i] = 0;
    }

    list_add(&r->listhead, run_queue);
    unlock();
    return r;
}

void thread_exit() {
    // set dead
    lock();
    curr_thread->status = DEAD;
    unlock();
    // move on
    schedule();
}

// timer
// set expired time
void schedule_timer(char *notuse) {
    unsigned long long cntfrq_el0;
    // Set the expired time as core timer frequency shift right 5 bits.
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t"
                         : "=r"(cntfrq_el0)); // tick frequency
    add_timer(schedule_timer, "", cntfrq_el0 >> 5, 1);
}