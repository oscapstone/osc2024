#include "sched.h"
#include "irq.h"
#include "malloc.h"
#include "timer.h"
#include "uart.h"
#include "current.h"
#include "signal.h"
#include "string.h"

thread_t *curr_thread;
list_head_t *run_queue;
list_head_t *wait_queue;
thread_t threads[PIDMAX + 1];

void init_thread_sched() {
    lock();
    // malloc run & wait queue
    run_queue = malloc(sizeof(list_head_t));
    wait_queue = malloc(sizeof(list_head_t));
    INIT_LIST_HEAD(run_queue);
    INIT_LIST_HEAD(wait_queue);

    // init pids
    // 類似 thread pool
    for (int i = 0; i <= PIDMAX; i++) {
        threads[i].pid = i;
        threads[i].status = FREE;
    }
    // set current tmp thread
    thread_t *tmp = malloc(sizeof(thread_t));
    // set tpidr_el1, 將tmp的context設置到tpidr_el1 reg。
    // gets the current thread’s data structure from the system register tpidr_el1.
    set_current_ctx(&tmp->context);
    curr_thread = tmp;

    // create idle
    thread_create(idle);
    // Enable the core timer interrupt.
    // Schedule the pending threads when the core timer interrupts.
    add_timer(schedule_timer, "", 1, 0);
    unlock();
}

void idle() {
    // In this lab check if there is any zombies
    // recycles zombies
    while (1) {   
        // reclaim threads marked as DEAD
        kill_zombies(); 
        // switch to next thread in run_queue
        schedule();     
    }
}

void schedule() {
    lock();
    // 判斷current thread是否等於run_queue或current thread是否DEAD
    do {
        curr_thread = (thread_t *)curr_thread->listhead.next;
    } while (list_is_head(&curr_thread->listhead, run_queue) || curr_thread->status == DEAD);
    // switch to next thread, store cuur_ctx, load next thread
    switch_to(current_ctx, &curr_thread->context);
    unlock();
}

void kill_zombies() {
    lock();
    list_head_t *now;
    // 搜尋 run_queue 並找到DEAD thread
    list_for_each(now, run_queue) {
        thread_t *cur_thread = (thread_t *)now;
        if (cur_thread->status == DEAD) {
            // delete from run queue  
            list_del(now);
            // free stack
            free(cur_thread->user_sp);   
            free(cur_thread->kernel_sp); 
            // set free
            cur_thread->status = FREE;
        }
    }
    unlock();
}

int exec_thread(char *data, unsigned int filesize) {
    // run syscall.img
    thread_t *t = thread_create(data);
    // malloc memory for data
    t->data = malloc(filesize);
    t->datasize = filesize;
    // set lr location
    t->context.lr = (unsigned long)t->data;
    // copy file into data
    memcpy(t->data, data, filesize);
    // set curr_thread
    lock();
    curr_thread = t;
    unlock();
    // eret to exception level 0
    // set tpidr to thread context_t
    // spsr_el1 
    // 0~3 bit 0b0000 : el0t , jump to el0 and use el0 stack
    // 6~9 bit 0b0000 : turn on every interrupt

    // elr_el1 : 設定program的起始位置給elr_el1
    
    // sp_el0 : user mode stack pointer, stack pointer set to top of program
    
    // sp : set kernel stack pointer, current is in el1 , so = sp_el1
    // stack往下長的
    // eret to context lr
    __asm__ __volatile__("msr tpidr_el1, %0\n\t" // 將 %0 (t->context)的值寫入 TPIDR_EL1
                         "msr elr_el1, %1\n\t"   // 將 %1 (t->data)的值寫入 ELR_EL1
                         "msr spsr_el1, xzr\n\t" // 將 xzr (清零)的值寫入 SPSR_EL1
                         "msr sp_el0, %2\n\t"    // 將 %2 (t->user_sp + USTACK_SIZE)的值寫入 SP_EL0
                         "mov sp, %3\n\t"        // 將 %3 (t->kernel_sp + KSTACK_SIZE)的值移動到 SP
                         "eret\n\t" ::"r"(&t->context), // return ELR_EL1 指定的地址，並切換到user mode
                         "r"(t->data), "r"(t->user_sp + USTACK_SIZE), "r"(t->kernel_sp + KSTACK_SIZE));

    return 0;
}

thread_t *thread_create(void *start) {
    lock();

    thread_t *r;
    // 找可用的thread
    for (int i = 0; i <= PIDMAX; i++) {
        if (threads[i].status == FREE) {
            r = &threads[i];
            break;
        }
    }
    // set status
    r->status = RUNNING;
    // set lr to function location
    r->context.lr = (unsigned long long)start;
    r->user_sp = malloc(USTACK_SIZE);
    r->kernel_sp = malloc(KSTACK_SIZE);
    // set stack and frame pointer pointer top
    r->context.sp = (unsigned long long)r->kernel_sp + KSTACK_SIZE;
    r->context.fp = r->context.sp;
    // init is checking
    r->signal_is_checking = 0;
    // initial signal handler with signal_default_handler (kill thread)
    for (int i = 0; i < SIGNAL_MAX; i++) {
        r->signal_handler[i] = signal_default_handler;
        r->sigcount[i] = 0;
    }
    // add to run_queue
    list_add(&r->listhead, run_queue);
    unlock();
    return r;
}

void thread_exit() {
    // set dead
    lock();
    // wait kill zombie to recollect
    curr_thread->status = DEAD;
    unlock();
    // move on (end curr_thread)
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
