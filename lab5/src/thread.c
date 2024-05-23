#include "thread.h"
#include "alloc.h"
#include "mini_uart.h"
#include "c_utils.h"
#include "syscall.h"
#include <stdint.h>

// define three queues
thread_t* running_q_head = 0;
thread_t* zombie_q_head  = 0;

thread_t* idle_thread = 0;

int cur_tid = 0;

void push(thread_t** head, thread_t* t) {
    if(!(*head)) {
        t -> prev = t;
        t -> next = t;
        (*head) = t;
    } else {
        t->prev = (*head)->prev;
        t->next = (*head);
        (*head)->prev->next = t;
        (*head)->prev = t;
    }
}

thread_t* pop(thread_t** head) {
    if(!(*head)) return 0;
    thread_t* res = (*head);
    if(res -> next == res){
        (*head) = 0;
    } else {
        res -> prev -> next = res -> next;
        res -> next -> prev = res -> prev;
        (*head) = res -> next;
    }

    return res;
}

void pop_t(thread_t** head, thread_t* t) {
    if(!(*head)) return;
    if(t -> next == t){
        (*head) = 0;
    } else {
        t -> prev -> next = t -> next;
        t -> next -> prev = t -> prev;
    }
}

void print_queue(thread_t* head) {
    uart_send_string("[INFO] Print Queue:\n[");
    thread_t* cur = head;
    do {
        uart_hex(cur->tid);
        uart_send_string(", ");
        cur = cur -> next;
    } while(cur != head);
    uart_send_string("]\n");
}

void schedule() {
    thread_t* cur_thread = get_current_thread();
    thread_t* next_thread = (cur_thread->state != TASK_RUNNING) ? running_q_head : cur_thread->next;
    uint64_t freq;
    // asm volatile("mrs %0, cntfrq_el0" : "=r" (freq));
    // uart_send_string("Timer frequency: ");
    // uart_hex(freq);
    // uart_send_string("\n");

    // asm volatile("mrs %0, cntp_tval_el0" : "=r" (freq));
    // uart_send_string("Timer value: ");
    // uart_hex(freq);
    // uart_send_string("\n");

    // asm volatile("mrs %0, cntp_ctl_el0" : "=r" (freq));
    // uart_send_string("Timer control: ");
    // uart_hex(freq);
    // uart_send_string("\n");
    if (next_thread->tid != cur_thread->tid) {
        // uart_send_string("[schedule] Switching from tid ");
        // uart_hex(cur_thread->tid);
        // uart_send_string(" to tid ");
        // uart_hex(next_thread->tid);
        // uart_send_string("\n");

        // uart_send_string("Current SP: ");
        // uart_hex(cur_thread->callee_reg.sp);
        // uart_send_string("\n");

        // uart_send_string("Next SP: ");
        // uart_hex(next_thread->callee_reg.sp);
        // uart_send_string("\n");
        switch_to(cur_thread, next_thread);
    }
    // switch to will switch to target thread by setting the lr 
    // if(cur_thread -> state != TASK_RUNNING) {
    //     uart_send_string("old sp: ");
    //     uart_hex(cur_thread -> callee_reg.sp);
    //     uart_send_string("\n");
    //     uart_send_string("new sp: ");
    //     uart_hex(running_q_head -> callee_reg.sp);
    //     uart_send_string("\n");
    //     switch_to(cur_thread, running_q_head);
    // } else {
    //     switch_to(cur_thread, cur_thread -> next);
    // }
}

void kill_zombies() {
    while(zombie_q_head) {
        uart_send_string("<ZOMBIE>: ");
        print_queue(zombie_q_head);
        thread_t* zombie = pop(&zombie_q_head);
        uart_send_string("[KILL] killing tid=");
        uart_hex(zombie->tid);
        uart_send_string("\n");
        kfree(zombie->user_stack);
        kfree(zombie->kernel_stack);
        kfree(zombie);
    }
}

thread_t* create_thread(void (*func)(void)) {
    thread_t* t = (thread_t*)kmalloc(sizeof(thread_t));
    t -> tid = cur_tid ++;
    t -> state = TASK_RUNNING;
    t -> callee_reg.lr = (unsigned long)func;
    t -> user_stack = kmalloc(T_STACK_SIZE);
    t -> kernel_stack = kmalloc(T_STACK_SIZE);
    t -> callee_reg.sp = (unsigned long)(t->user_stack + T_STACK_SIZE);
    t -> callee_reg.fp = t -> callee_reg.sp; // set fp to sp as the pointer that fixed

    push(&running_q_head, t);
    print_queue(running_q_head);
    return t;
}


thread_t* get_thread_from_tid(int tid) {
    thread_t* cur = running_q_head;
    do {
        if(cur -> tid == tid) {
            return cur;
        }
        cur = cur -> next;
    } while(cur != running_q_head);

    return 0;
}

void kill_thread(int tid) {
    thread_t* t = get_thread_from_tid(tid);
    if(!t) return;
    t -> state = TASK_ZOMBIE;
    running_q_head = t -> next;
    pop_t(&running_q_head, t);
    push(&zombie_q_head, t);
    uart_send_string("[kill]\n");
    schedule();
}

void thread_init() {
    uart_send_string("[INFO] Init thread\n");
    idle_thread = create_thread(idle);
    asm volatile("msr tpidr_el1, %0" :: "r"(idle_thread));
}

void thread_exit() {
    thread_t* cur = get_current_thread();
    cur -> state = TASK_ZOMBIE;
    running_q_head = cur -> next;
    pop_t(&running_q_head, cur);
    push(&zombie_q_head, cur);
    schedule();
}

void idle() {
    while(1) {
        kill_zombies();
        // print_queue(running_q_head);
        schedule();
        
    }
}

void foo(){
    for(int i = 0; i < 10; ++i) {
        uart_send_string("Thread id: ");
        uart_hex(get_current_thread()->tid);
        uart_send_string(" ");
        uart_hex(i);
        uart_send_string("\n");
        delay(1000000);
        schedule();
    }
    thread_exit();
}

void thread_test() {
    for(int i=0;i<3;i++) {
        uart_send_string("[INFO] Thread id: ");
        uart_hex(i);
        uart_send_string("\n");
        create_thread(foo);
    }
    // print_queue(running_q_head);
    idle();
}

void run_fork_test() {
    thread_t* t = get_current_thread();
    uart_hex(t->tid);
    uart_send_string("\n");
    uart_hex(t->callee_reg.sp);
    // delay(1000000);
    for(int i=0;i<1000000;i++);
    asm volatile("msr spsr_el1, %0" ::"r"(0x3c0));         // disable E A I F
    asm volatile("msr elr_el1, %0" ::"r"(main_fork_test));       // get back to caller function
    asm volatile("msr sp_el0, %0" ::"r"(t->callee_reg.sp));
    asm volatile("mov sp, %0" ::"r"(t->kernel_stack + T_STACK_SIZE));
    asm volatile("eret");
}

void main_fork_test(){
    uart_send_string("\nFork Test, pid: ");
    uart_hex(sys_getpid());
    uart_send_string("\n");
    int cnt = 1;
    int ret = 0;
    if ((ret = sys_fork()) == 0) { // child
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        uart_send_string("first child pid: ");
        uart_hex(sys_getpid());
        uart_send_string(", cnt:");
        uart_hex(cnt);
        uart_send_string(", ptr:");
        uart_hex(&cnt);
        uart_send_string(", sp: ");
        uart_hex(cur_sp);
        uart_send_string("\n");
        ++cnt;

        if ((ret = sys_fork()) != 0){
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            uart_send_string("first child pid: ");
            uart_hex(sys_getpid());
            uart_send_string(", cnt:");
            uart_hex(cnt);
            uart_send_string(", ptr:");
            uart_hex(&cnt);
            uart_send_string(", sp: ");
            uart_hex(cur_sp);
            uart_send_string("\n");
        }
        else {
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                uart_send_string("second child pid: ");
                uart_hex(sys_getpid());
                uart_send_string(", cnt: ");
                uart_hex(cnt);
                uart_send_string(", ptr: ");
                uart_hex(&cnt);
                uart_send_string(", sp: ");
                uart_hex(cur_sp);
                uart_send_string("\n");
                for(int i=0;i<1000000;i++);
                ++cnt;
            }
        }
        sys_exit(0);
    }
    else {
        uart_send_string("parent here, pid ");
        uart_hex(sys_getpid());
        uart_send_string(", child ");
        uart_hex(ret);
        uart_send_string("\n");
        print_queue(running_q_head);
    }
    sys_exit(0);
}

void fork_test() {
    create_thread(run_fork_test);
    idle();
}