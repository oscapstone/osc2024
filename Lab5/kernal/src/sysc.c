#include"scheduler.h"
#include"sysc.h"
#include"stddef.h"
#include"cpio.h"
#include"lock.h"
#include"memalloc.h"
#include"irq.h"
#include"mailbox.h"
int sys_getpid(trapframe_t* tf){
    tf->x0=running_ring->pid;
    return tf->x0;
}

unsigned long sys_uart_read(trapframe_t* tf){
    char* buf=tf->x0;
    unsigned long size=tf->x1;
    int i=0;
    asm volatile(
            "msr daifclr, 0xf;"
        );
    for(;i<size;i++){
        buf[i]=getchar();
    }
    asm volatile(
        "msr daifset, 0xf;"
    );
    tf->x0=i;
    return tf->x0; 
}

unsigned long sys_uart_write(trapframe_t* tf){
    char* buf=tf->x0;
    unsigned long size=tf->x1;
    // tf->x0=uart_irq_puts(buf);
    // return 0;
    tf->x0=puts_len(buf,size);
    return 0;
}

int sys_exec(trapframe_t* tf){
    char* name=tf->x0;
    char** argv=tf->x1;
    puts("allocate process text:\r\n");
    void* proc=fr_malloc(256*1024);
    if(cpio_load(name,proc)){
        puts("sys_exec loadimg fault\r\n");
        return 1;
    }
    running_ring->proc=proc;
    tf->elr_el1=running_ring->proc;
    tf->sp_el0=(void*)running_ring+KSTACK_SIZE+USTACK_SIZE;
    tf->x0=0;

    return 0;
}

void sys_fork(trapframe_t* tf){
    // puts("parent trapframe:");
    // output_trapframe(tf);
    // puts("in fork: ");
    // output_daif();
    TCB* child=thread_create(running_ring->proc);
    TCB* parent=running_ring;
    char* child_base=(char*)child;
    char* parent_base=(char*)parent;
    for(unsigned long offset=sizeof(TCB);offset<KSTACK_SIZE+USTACK_SIZE;offset++){
        child_base[offset]=parent_base[offset];
    }
    
    // puts("point2 ");puts("id:");put_int(running_ring->pid);
    // context_output(&(running_ring->context));
    store_context(get_current());
    // puts("point3 ");puts("id:");put_int(running_ring->pid);
    // context_output(&(running_ring->context));
    if(running_ring != parent){
        // trapframe_t* child_tf=child_base+((char*)tf - parent_base);
        // puts("child trapframe:");
        // output_trapframe(child_tf);
        return;
    }

    trapframe_t* child_tf=child_base+((char*)tf - parent_base);
    child_tf->x0=0;
    // puts("parent tf:");
    // output_trapframe(tf);
    child_tf->sp_el0=child_base+((char*)tf->sp_el0 - parent_base);
    // puts("child_sp_el0:");
    // put_hex(child_tf->sp_el0);
    // puts("\r\n");
    tf->x0=child->pid;
    child->context=parent->context;
    child->context.sp=child_base+((char*)parent->context.sp - parent_base);
    child->context.fp=child_base+((char*)parent->context.fp - parent_base);
    //child->context.lr=parent->context.lr;
    store_context(get_current());
    // puts("point4 ");puts("id:");put_int(running_ring->pid);
    // context_output(&(running_ring->context));
    // puts("child trapframe:");
    // output_trapframe(child_tf);
    // puts("parent trapframe:");
    // output_trapframe(tf);
    //reset(0x400);
    return;
}

int sys_exit(trapframe_t* tf){
    thread_exit();
    return 0;
}

int sys_mbox_call(trapframe_t *tf)
{
    tf->x0=mbox_call_buf((tf->x0),tf->x1);
    return tf->x0;
}

void sys_kill(trapframe_t *tf){
    int kill_id=tf->x0;
    TCB* current=running_ring;
    while(current->pid != kill_id){
        current=current->next;
        if(current == running_ring){
            puts("kill: have no target pid process\r\n");
            return 1;
        }
    }
    TCB* killed=current;
    if(killed == running_ring)exit();
    killed->next->prev=killed->prev;
    killed->prev->next=killed->next;
    killed->prev=NULL;
    killed->next=zombie_list;
    zombie_list=killed;
    return;
}