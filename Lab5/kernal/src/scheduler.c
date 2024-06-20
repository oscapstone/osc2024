#include "scheduler.h"
#include "utils.h"
#include "memalloc.h"
#include "lock.h"
#include "stddef.h"
#include "str.h"
#include "mini_uart.h"
TCB* running_ring=NULL;
TCB* zombie_list=NULL;


int thread_init(){
    thread_create(idle);

    asm("msr tpidr_el1, %0\n\t" // Hold the "kernel(el1)" thread structure information
        "msr elr_el1, %1\n\t"   // When el0 -> el1, store return address for el1 -> el0
        "msr spsr_el1, xzr\n\t" // Enable interrupt in EL0 -> Used for thread scheduler
        "msr sp_el0, %2\n\t"    // el0 stack pointer for el1 process
        "mov sp, %3\n\t"        // sp is reference for the same el process. For example, el2 cannot use sp_el2, it has to use sp to find its own stack.
        "eret\n\t" ::"r"(&running_ring->context),"r"(running_ring->context.lr), "r"((char*)(running_ring->u_stack_allo_base)+USTACK_SIZE), "r"((char*)(running_ring->k_stack_allo_base) + KSTACK_SIZE));
    puts("should not output this message\r\n");
    return 1;
}

void idle(){
    exec("syscall.img");
    int key=5678;
    puts("before fork,key address:");
    put_hex(&key);
    puts("\r\n");
    key=fork();
    if(key){
        while(1){
            
            puts("this is idle parent thread,pid:");
            put_int(getpid());
            puts("\r\n");
            // puts(" sp:");
            // put_hex(get_sp());
            // puts("\r\n");
            // puts("\r\nkeys:");
            // put_int(key);
            // puts(" keys address:");
            // put_hex(&key);
            // puts("\r\n");
            // context_output(&(running_ring->context));
            // output_lockstate();
            delay(1000000);
            // uart_write("do you want to reboot? :",sizeof("do you want to reboot? :"));
            // uart_read(arr,32);
            // if(strcmp(arr,"reboot") == 0){
            //     reset(0x400);CurrentEL
            // }
            //kill_zombies();
            //schedule();
            //puts("after ashedule\r\n");
        }
    }
    else{
        //uart_irq_on();
        exec("syscall.img");
        while(1){
            puts("this is idle child thread,pid:");
            put_int(getpid());
            puts(" sp:");
            put_hex(get_sp());
            puts("\r\n");
            // puts("\r\nkeys:");
            // put_int(key);
            // puts(" keys address:");
            // put_hex(&key);
            // puts("\r\n");
            // context_output(&(running_ring->context));
            // output_lockstate();
            

            delay(1000000);
            
            // uart_write("do you want to reboot? :",sizeof("do you want to reboot? :"));
            // uart_read(arr,32);
            // if(strcmp(arr,"reboot") == 0){
            //     reset(0x400);CurrentEL
            // }
            //kill_zombies();
            //schedule();
            //puts("after ashedule\r\n");
        }
    }
    
    
    return;
}

int schedule(){
    //puts("in schedule:");output_daif();
    static int first=1;
    unsigned long long current_el;
    asm volatile ("mrs %0, CurrentEL" : "=r" (current_el));
    current_el >> 2;
    if(current_el <= 0){
        puts("error: in el0\r\n");
        return 0;
    }
    if(running_ring == NULL){
        puts("error:schedule case 1 \r\n");
        reset(0x800);
    }
    // if(first){
    //     first=0;
    //     puts("first schedule\r\n");
    //     load_context(&(running_ring->context));
    // }
    thread_context_t* current_context=&(running_ring->context);
    thread_context_t* next_context=&(running_ring->next->context);
    running_ring=running_ring->next;
    switch_to(current_context,next_context);
    return 0;
}

int kill_zombies(){ //should run in el1
    puts("killing\r\n");
    while(zombie_list != NULL){
        TCB* freed=zombie_list;
        zombie_list=zombie_list->next;
        fr_free(freed);
    }
    puts("killing end\r\n");
    return 0;
}

int pid_generator(){
    static unsigned int id=0;
    unsigned int pid=id++;
    return pid;
}

TCB* thread_create(void* proc_start){
    TCB* new_entry=fr_malloc(KSTACK_SIZE+USTACK_SIZE);
    // puts("new_entry:");
    // put_hex(new_entry);
    // puts("\r\n");
    new_entry->k_stack_allo_base=new_entry;
    new_entry->u_stack_allo_base=(unsigned long)(new_entry)+KSTACK_SIZE;
    new_entry->next=new_entry;
    new_entry->prev=new_entry;
    new_entry->pid=pid_generator();
    new_entry->proc=proc_start;
    new_entry->context.lr=proc_start;
    new_entry->context.sp=(unsigned long)new_entry+KSTACK_SIZE;
    // puts("in thread_create,sp:");
    // put_hex(new_entry->context.sp);
    // puts("\r\n");
    new_entry->context.fp=new_entry->context.sp;
    if(running_ring == NULL){
        running_ring=new_entry;
    }
    else{
        new_entry->next=running_ring;
        new_entry->prev=running_ring->prev;
        new_entry->next->prev=new_entry;
        new_entry->prev->next=new_entry;
    }
    return new_entry;
}

void foo3(){
    fork();
    puts("Thread id: ");
    
    put_int(getpid());
    puts("\r\n");
    delay(1000000);
    
    

    exit();
}

void foo2(){
    for(int i=0;i<10;i++){
        
        puts("Thread id: ");
        
        put_int(getpid());
        
        puts(" time:");
        put_int(i);
        puts("\r\n");
        
        delay(1000000);
        if(i == 5)fork();
        schedule();
    }

    exit();
}

void foo4(){
    int doge=0;
    puts("point1 ");puts("id:");put_int(running_ring->pid);
    context_output(&(running_ring->context));
    if(fork()){
        for(int i=0;i<10;i++){
            doge++;
            puts("this is parent,Thread id: ");
            
            put_int(getpid());
            
            puts(" time:");
            put_int(doge);
            puts("\r\n");
            store_context(get_current());
            context_output(get_current());
            delay(1000000);
            schedule();
        }
    }
    else{
        for(int i=0;i<10;i++){
            doge--;
            puts("this is child,Thread id: ");
            
            put_int(getpid());
            
            puts(" time:");
            put_int(doge);
            puts("\r\n");
            store_context(get_current());
            context_output(get_current());
            delay(1000000);
            schedule();
        }
    }
    exit();
}

void foo(){
    char name[15]="syscall.img";
    exec(name);
}

void thread_exit(){
    TCB* killed=running_ring;
    running_ring=running_ring->next;
    killed->next->prev=killed->prev;
    killed->prev->next=killed->next;
    killed->prev=NULL;
    killed->next=zombie_list;
    zombie_list=killed;
    load_context(&(running_ring->context));
    return;
}

void thread_ecec(char* name){
    exec(name);
}

void context_output(thread_context_t* context){
    unsigned long* arr=context;
    // for(int i=0;i<10;i++){
    //     puts(" x");
    //     put_int(i+19);
    //     puts(":");
    //     put_hex(arr[i]);
    // }
    puts(" fp:");
    put_hex(arr[10]);
    puts(" lr:");
    put_hex(arr[11]);
    puts(" sp:");
    put_hex(arr[12]);
    puts("\r\n");
    return;
}

void thread_exec(){
}

void fork_test(){
    puts("\r\nFork Test, pid ");
    put_int(getpid());
    puts("\r\n");
    int cnt = 1;
    int ret = 0;
    if ((ret = fork()) == 0) { // child
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        puts("first child pid: ");
        put_int(getpid());
        puts(", cnt: ");
        put_int(cnt);
        puts(", ptr: ");
        put_hex(&cnt);
        puts(", sp : ");
        put_hex(cur_sp);
        puts("\r\n");

        ++cnt;

        if ((ret = fork()) != 0){
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            puts("first child pid: ");
            put_int(getpid());
            puts(", cnt: ");
            put_int(cnt);
            puts(", ptr: ");
            put_hex(&cnt);
            puts(", sp : ");
            put_hex(cur_sp);
            puts("\r\n");
        }
        else{
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                puts("second child pid: ");
                put_int(getpid());
                puts(", cnt: ");
                put_int(cnt);
                puts(", ptr: ");
                put_hex(&cnt);
                puts(", sp : ");
                put_hex(cur_sp);
                puts("\r\n");
                delay(1000000);
                ++cnt;
            }
        }
        exit();
    }
    else {
        puts("parent here, pid ");
        put_int(getpid());
        puts(", child ");
        put_int(ret);
        puts("\r\n");
        exit();
    }
    
}