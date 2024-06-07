#include "scheduler.h"
#include "utils.h"
#include "memalloc.h"
#include "lock.h"
#include "stddef.h"
#include "str.h"
TCB* running_ring=NULL;
TCB* zombie_list=NULL;


int thread_init(){
    thread_create(idle);
    for(int i=0;i<5;i++){
        thread_create(foo);
    }
    schedule();
    puts("should not output this message\r\n");
    return 1;
}

void idle(){
    int counter=0;
    while(1){
        puts("this is idle thread,sp:");
        put_hex(get_sp());
        puts("\r\n");
        delay(1000000);
        if(running_ring->next == running_ring){
            counter++;
        }
        else counter=0;
        if(counter >= 50)reset(0x400);
        // uart_write("do you want to reboot? :",sizeof("do you want to reboot? :"));
        // uart_read(arr,32);
        // if(strcmp(arr,"reboot") == 0){
        //     reset(0x400);
        // }
        kill_zombies();
        schedule();
    }
    
    return;
}

int schedule(){
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
    if(first){
        first=0;
        puts("first schedule\r\n");
        load_context(&(running_ring->context));
    }
    thread_context_t* current_context=&(running_ring->context);
    thread_context_t* next_context=&(running_ring->next->context);
    running_ring=running_ring->next;
    switch_to(current_context,next_context);
    return 0;
}

int kill_zombies(){
    while(zombie_list != NULL){
        TCB* freed=zombie_list;
        zombie_list=zombie_list->next;
        fr_free(freed);
    }
    return 0;
}

int pid_generator(){
    static unsigned int id=0;
    lock();
    unsigned int pid=id++;
    unlock();
    return pid;
}

TCB* thread_create(void* proc_start){
    TCB* new_entry=fr_malloc(KSTACK_SIZE+USTACK_SIZE);
    puts("new_entry:");
    put_hex(new_entry);
    puts("\r\n");
    new_entry->k_stack_allo_base=new_entry;
    new_entry->u_stack_allo_base=new_entry+KSTACK_SIZE;
    new_entry->next=new_entry;
    new_entry->prev=new_entry;
    new_entry->pid=pid_generator();
    new_entry->proc=proc_start;
    new_entry->context.lr=proc_start;
    new_entry->context.sp=(unsigned long)new_entry+KSTACK_SIZE;
    puts("in thread_create,sp:");
    put_hex(new_entry->context.sp);
    puts("\r\n");
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

void foo(){
    for(int i = 0; i < 10; ++i) {
        puts("Thread id: ");
        put_int(getpid());
        puts(" time:");
        put_int(i);
        puts("\r\n");
        delay(1000000);
        schedule();
    }
    exit();
}

void thread_exit(){
    lock();
    TCB* killed=running_ring;
    running_ring=running_ring->next;
    killed->next->prev=killed->prev;
    killed->prev->next=killed->next;
    killed->prev=NULL;
    killed->next=zombie_list;
    zombie_list=killed;
    unlock();
    load_context(&(running_ring->context));
    return;
}