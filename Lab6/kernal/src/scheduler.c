#include "scheduler.h"
#include "utils.h"
#include "memalloc.h"
#include "lock.h"
#include "stddef.h"
#include "str.h"
#include "mini_uart.h"
#include"peripherals/rpi_mmu.h"
#include "cpio.h"
#include "mmu.h"
TCB* running_ring=NULL;
TCB* zombie_list=NULL;


int thread_init(){
    puts("in thread_init\r\n");
    img_exec("vm.img");
    return 1;
}

void idle(){
    //exec("syscall.img");
    int key=5678;
    puts("before fork,key address:");
    put_hex(&key);
    puts("\r\n");
    //key=fork();
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
        //exec("syscall.img");
        while(1){
            puts("this is idle child thread,pid:");
            put_int(getpid());
            puts(" sp:");
            put_long_hex(get_sp());
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
    //puts("switching\r\n");
    thread_context_t* current_context=&(running_ring->context);
    thread_context_t* next_context=&(running_ring->next->context);
    running_ring=running_ring->next;
    switch_to(current_context,next_context);
    return 0;
}

int kill_zombies(){ //should run in el1
    //puts("killing\r\n");
    while(zombie_list != NULL){
        TCB* freed=zombie_list;
        zombie_list=zombie_list->next;
        fr_free(freed);
    }
    //puts("killing end\r\n");
    return 0;
}

int pid_generator(){
    static unsigned int id=0;
    unsigned int pid=id++;
    return pid;
}

int img_exec(char* filepath){
    void* proc_base=fr_malloc(cpio_size(filepath));
    cpio_load(filepath,proc_base);
    TCB* new_thread=thread_create(proc_base,cpio_size(filepath));
    puts("new_thread->context.pgd:");
    put_long_hex(new_thread->context.pgd);
    puts("\r\n");
    map_pages(PHYS_TO_VIRT(new_thread->context.pgd),USER_PROC_BASE,VIRT_TO_PHYS(new_thread->proc),cpio_size(filepath),USER_ATTR_NORMAL_NOCACHE);
    map_pages(PHYS_TO_VIRT(new_thread->context.pgd),PERIPHERAL_START,PERIPHERAL_START,PERIPHERAL_END-PERIPHERAL_START,USER_ATTR_nGnRnE);
    map_pages(PHYS_TO_VIRT(new_thread->context.pgd),0xffffffffb000,VIRT_TO_PHYS(new_thread->u_stack_allo_base+USTACK_SIZE-0x4000),0x4000,USER_ATTR_NORMAL_NOCACHE);
    puts("goto el0\r\n");

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
        "eret\n\t" ::"r"(&new_thread->context),"r"(USER_PROC_BASE), "r"(USER_STACK_BASE), "r"(new_thread->k_stack_allo_base + KSTACK_SIZE), "r"(new_thread->context.pgd));

    puts("should not output this line\r\n");
    return 0;
}

TCB* thread_create(void* proc_start,unsigned int proc_size){
    if(((unsigned long)proc_start & 0xfff) != 0){puts("error:thread create proc_start wrong\r\n"); return NULL;} 
    TCB* new_entry=fr_malloc(KSTACK_SIZE+USTACK_SIZE);
    puts("address of new_entry:");
    put_long_hex(new_entry);
    puts("\r\n");
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
    new_entry->proc_size=proc_size;
    
    new_entry->context.pgd=VIRT_TO_PHYS((unsigned long)get_new_page());
    puts("in thread_create,new_entry->context.pgd:");
    put_long_hex(new_entry->context.pgd);
    puts("\r\n");
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

void foo(){
    char name[15]="syscall.img";
    exec(name);
}

void thread_exit(){
    TCB* killed=running_ring;
    thread_context_t* current_context=&(running_ring->context);
    thread_context_t* next_context=&(running_ring->next->context);
    running_ring=running_ring->next;
    killed->next->prev=killed->prev;
    killed->prev->next=killed->next;
    killed->prev=NULL;
    killed->next=zombie_list;
    zombie_list=killed;
    switch_to(current_context,next_context);
    //load_context(&(running_ring->context));
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
    put_long_hex(arr[10]);
    puts(" lr:");
    put_long_hex(arr[11]);
    puts(" sp:");
    put_long_hex(arr[12]);
    puts("\r\n");
    return;
}