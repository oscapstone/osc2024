#include"scheduler.h"
#include"sysc.h"
#include"stddef.h"
#include"cpio.h"
#include"lock.h"
#include"memalloc.h"
#include"irq.h"
int sys_getpid(trapframe_t* tf){
    tf->x0=running_ring->pid;
    return tf->x0;
}

unsigned long sys_uart_read(trapframe_t* tf){
    char* buf=tf->x0;
    unsigned long size=tf->x1;
    
    return get(buf,size);
}

unsigned long sys_uart_write(trapframe_t* tf){
    char* buf=tf->x0;
    unsigned long size=tf->x1;
    puts_len(buf,size);
}

int sys_exec(trapframe_t* tf){
    char* name=tf->x0;
    char** argv=tf->x1;
    running_ring->proc=(void*)running_ring+sizeof(TCB);
    running_ring=mem_alin(running_ring,32);
    if((void*)running_ring->proc < (void*)running_ring || (void*)running_ring->proc > (void*)running_ring+(USTACK_SIZE+KSTACK_SIZE)){
        puts("text can't overwrite\r\n"); //text isn't in thread user stack or kernel stack
        return 1;
    }
    if(cpio_load(name,running_ring->proc)){
        puts("sys_exec loadimg fault\r\n");
        return 1;
    }
    tf->elr_el1=running_ring->proc;
    tf->sp_el0=(void*)running_ring+KSTACK_SIZE+USTACK_SIZE;
    tf->x0=0;

    return 0;
}

int sys_fork(trapframe_t* tf){
    TCB* child=thread_create(running_ring->proc);
}

int sys_exit(trapframe_t* tf){
    thread_exit();
    return 0;
}