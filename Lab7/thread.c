#include "memory.h"
#include "thread.h"
#include "uart.h"
#include "vfs.h"

// kernel stack: trapframe,... etc
// user stack: user variables ...

extern void switch_to();
extern thread* get_current();
int min_priority = 999;

#define MAX_TASK 64
thread * thread_pool[MAX_TASK];

void update_min_priority(){
    min_priority = 999;
    for(int i=0; i<MAX_TASK; i++){
        if(thread_pool[i] == 0 || thread_pool[i] -> state != 1)
            continue;
        if(min_priority > thread_pool[i] -> priority)
            min_priority = thread_pool[i] -> priority;
    }
}

void thread_exit(){
    //set state to the end and schedule
    thread * cur = get_current();
    cur -> state = -1; //end
    if(cur -> priority == min_priority){
        cur -> priority = 999;
        update_min_priority();
    }
    schedule();
}

void thread_execute(){
    // pack the function for state management
    uart_puts("In exec pid: ");
    uart_int(get_current() -> pid);
    newline();
    thread * cur = get_current();
    cur -> funct();
    uart_puts("End pid: ");
    uart_int(get_current() -> pid);
    newline();
    thread_exit();
}

int create_thread(void * function, int priority){
    int pid;
    thread * t = allocate_page(sizeof(thread));//malloc(sizeof(struct thread));
    for(int i=0; i<MAX_TASK; i++){
        if(thread_pool[i] == 0){
            pid = i;
            uart_puts("Create thread with PID ");
            uart_int(i);
            newline();
            break;
        }
        if(i == MAX_TASK - 1){
            uart_puts("Error, no more threads\n\r");
            return -1;
        }
    }

    memset(t, sizeof(struct thread));

    t -> pid = pid;
    t -> state = 1; //running
    t -> parent = -1;
    t -> priority = priority; //lower means higher in my implementation (Confusing XD)
    t -> funct = function;
    //add 4096(PAGE_SIZE) because stack grows up
    t -> sp_el1 = ((unsigned long)allocate_page(4096)) + 4096; //kernel stack for sp, trapframe
    t -> sp_el0 = ((unsigned long)allocate_page(4096)) + 4096; //user stack for user program
    t -> regs.lr = thread_execute; // ret jumps to lr -> switch to will load lr and call ret
    t -> regs.sp = t -> sp_el1;
    t -> preempt = 1;
    strcpy("/", t -> work_dir);
    update_min_priority();
    thread_pool[pid] = t; 
    vfs_open("/dev/uart", 0, &t->file_table[0]); // stdin
    vfs_open("/dev/uart", 0, &t->file_table[1]); // stdout
    vfs_open("/dev/uart", 0, &t->file_table[2]); 
    return pid;
}

int get_pid(){
    return get_current() -> pid;//no runnning thread
}

void kill_zombies(){
    //free all alocated memory for zombie threads and set thread pid to NULL
    for(int i=0;i<MAX_TASK;i++){
        if(thread_pool[i] == 0)
            continue;
        if(thread_pool[i] -> state == -1){//recycle thread with state -1
            free_page(thread_pool[i] -> sp_el0 - 4096);
            free_page(thread_pool[i] -> sp_el1 - 4096);
            free_page(thread_pool[i]);
            thread_pool[i] = 0;
            uart_puts("Killed zombie PID ");
            uart_int(i);
            newline();
        }
    }
}

void schedule(){
    if(get_current() -> preempt == 0 && get_current() -> state == 1)
        return;
    get_current() -> preempt = 0;
    update_min_priority();
    int next = get_current() -> pid;

    //find min priority thread closest to current thread
    for(int i = get_current() -> pid + 1; i<MAX_TASK; i++){ 
        if(thread_pool[i] == 0 || thread_pool[i] -> state != 1 || thread_pool[i] -> priority > min_priority)
            continue;
        next = i;
        break;
    }

    //start from left most if didn't find in right
    if(next == get_current() -> pid){
        for(int i = 0; i<get_current() -> pid; i++){
            if(thread_pool[i] == 0 || thread_pool[i] -> state != 1 || thread_pool[i] -> priority > min_priority) 
                continue;
            next = i;
            break;
        }
    }

    //switch to next thread
    if(get_current() -> pid != next && thread_pool[next] != 0 && thread_pool[next] -> state == 1){
        thread * from = get_current();
        thread * to = thread_pool[next];
        switch_to(from, to); //x0: from, x1: to, store regs to struct and load regs from struct
        // uart_puts("In PID: ");
        // print_pid();
        // uart_puts("after switch\n\r");
        // newline();
    }
    get_current() -> preempt = 1; //ex: selected the to thread, but preemptted and to has been modified
}

void idle(){
    //infinite loop for scheduling
    while(1){
        kill_zombies();
        schedule();
    }
}

void thread_init(){
    //make main process as a thread, will be placed in thread 0
    min_priority = 10;
    create_thread(0, 10);
    asm volatile ("msr tpidr_el1, %0"::"r"((unsigned long)thread_pool[0]));
}
