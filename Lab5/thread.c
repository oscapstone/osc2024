#include "memory.h"
#include "thread.h"
#include "uart.h"

//kernel stack: trapframe,... etc
//user stack: user variables ...

//set the start running address of thread to lr
struct registers {
	unsigned long x19;
	unsigned long x20;
	unsigned long x21;
	unsigned long x22;
	unsigned long x23;
	unsigned long x24;
	unsigned long x25;
	unsigned long x26;
	unsigned long x27;
	unsigned long x28;
	unsigned long fp;
	unsigned long lr;
	unsigned long sp;
};

struct thread{
    struct registers regs;
    int pid;
    int state; //run queue, running, wait queue... 0: created
    int parent; //-1: no parent
    int priority; //2 normal
    void (*funct)(void);
    unsigned long stack_pointer;
};

typedef struct thread thread;

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

void thread_execute(){
    thread * cur = get_current();
    cur -> funct();
    cur -> state = -1; //end
    if(cur -> priority == min_priority){
        cur -> priority = 999;
        update_min_priority();
    }
    schedule();
}

int create_thread(void * function){
    int pid;
    //give it a page, page minus header is free space
    thread * t = allocate_page(sizeof(thread));//malloc(sizeof(struct thread));
    for(int i=0; i<MAX_TASK; i++){
        if(thread_pool[i] == 0){
            pid = i;
            uart_puts("Create thread with PID ");
            uart_int(i);
            newline();
            break;
        }
    }

    for(int i = 0; i< sizeof(struct registers); i++){
        ((char*)(&(t -> regs)))[i] = 0; 
    }

    t -> pid = pid;
    t -> state = 1; //running
    t -> parent = -1;
    t -> priority = 2;
    t -> funct = function;
    t -> stack_pointer = ((unsigned long)t + 4096); //sp start from bottom
    t -> regs.lr = thread_execute;
    t -> regs.sp = t -> stack_pointer;
    update_min_priority();
    thread_pool[pid] = t; 
    return pid;
}

int get_pid(){
    return get_current() -> pid;//no runnning thread
}

void kill_zombies(){
    for(int i=0;i<MAX_TASK;i++){
        if(thread_pool[i] == 0)
            continue;
        if(thread_pool[i] -> state == -1){//recycle thread
            free_page(thread_pool[i]);
            thread_pool[i] = 0;
        }
    }
}

void schedule(){
    update_min_priority();
    int next = get_current() -> pid;
    for(int i = get_current() -> pid + 1; i<MAX_TASK; i++){
        if(thread_pool[i] == 0 || thread_pool[i] -> state != 1 || thread_pool[i] -> priority > min_priority)
            continue;
        next = i;
        break;
    }

    if(next == get_current() -> pid){
        for(int i = 0; i<get_current() -> pid; i++){
            if(thread_pool[i] == 0 || thread_pool[i] -> state != 1 || thread_pool[i] -> priority > min_priority)
                continue;
            next = i;
            break;
        }
    }

    if(get_current() -> pid != next || thread_pool[next] != 0 || thread_pool[next] -> state == 1){
        thread * from = get_current();
        thread * to = thread_pool[next];
        switch_to(from, to);
    }
    // else{
    //     uart_int(get_current() -> pid);
    //     uart_getc();
    // }
}

void idle(){
    while(1){
        kill_zombies();
        schedule();
    }
}

void thread_init(){ // a nop thread for idle
    min_priority = 10;
    thread * t = allocate_page(sizeof(thread));
    for(int i = 0; i< sizeof(struct registers); i++){
        ((char*)(&(t -> regs)))[i] = 0;
    }
    t -> state = 1;
    t -> parent = -1;
    t -> priority = 10;
    t -> funct = 0;
    t -> stack_pointer = 0;
    thread_pool[0] = t;
    asm volatile ("msr tpidr_el1, %0"::"r"((unsigned long)thread_pool[0]));
	unsigned long sp_el0 = 0;
	asm volatile ("msr sp_el0, %0"::"r"(sp_el0));
}
