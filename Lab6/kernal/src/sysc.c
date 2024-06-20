#include"scheduler.h"
#include"sysc.h"
#include"stddef.h"
#include"cpio.h"
#include"lock.h"
#include"memalloc.h"
#include"irq.h"
#include"mailbox.h"
#include"mmu.h"
#include"peripherals/rpi_mmu.h"
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
        if(buf[i] == '\n')break;
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
    void* proc=fr_malloc(cpio_size(name));
    if(cpio_load(name,proc)){
        puts("sys_exec loadimg fault\r\n");
        return 1;
    }
    running_ring->proc=proc;
    running_ring->proc_size=cpio_size(name);
    running_ring->context.pgd=VIRT_TO_PHYS((unsigned long)get_new_page());
    map_pages(PHYS_TO_VIRT(running_ring->context.pgd),USER_PROC_BASE,VIRT_TO_PHYS(running_ring->proc),cpio_size(name),USER_ATTR_NORMAL_NOCACHE);
    map_pages(PHYS_TO_VIRT(running_ring->context.pgd),PERIPHERAL_START,PERIPHERAL_START,PERIPHERAL_END-PERIPHERAL_START,USER_ATTR_nGnRnE);
    map_pages(PHYS_TO_VIRT(running_ring->context.pgd),0xffffffffb000,VIRT_TO_PHYS(running_ring->u_stack_allo_base+USTACK_SIZE-0x4000),0x4000,USER_ATTR_NORMAL_NOCACHE);

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
        "eret\n\t" ::"r"(&running_ring->context),"r"(USER_PROC_BASE), "r"(USER_STACK_BASE), "r"(running_ring->k_stack_allo_base + KSTACK_SIZE), "r"(running_ring->context.pgd));

    puts("should not output this line\r\n");
    tf->elr_el1=USER_PROC_BASE;
    tf->sp_el0=USER_STACK_BASE;
    tf->x0=0;

    return 0;
}

void sys_fork(trapframe_t* tf){
    // puts("parent trapframe:");
    // output_trapframe(tf);
    // puts("in fork: ");
    // output_daif();
    TCB* child=thread_create(running_ring->proc,running_ring->proc_size);
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
        puts("child start:");
        // puts("child pgd:");
        // put_long_hex(child->context.pgd);
        // puts("\r\n");
        // puts("current pgd:");
        // unsigned long ttbr0_el1=0x1234;
	    // asm volatile("mrs %0, ttbr0_el1":"=r"(ttbr0_el1));
        // put_long_hex(ttbr0_el1);
        // puts("\r\n");
        // puts("child id:");
        // put_int(child->pid);
        // puts("\r\n");
        // puts("running thread id:");
        // put_int(running_ring->pid);
        // puts("\r\n");
        // puts("child prev:");
        // put_long_hex(child->prev);
        // puts("\r\n");
        // puts("child next:");
        // put_long_hex(child->next);
        // puts("\r\n");
        // puts("parent pgd:");
        // put_long_hex(parent->context.pgd);
        // puts("\r\n");
        // va_to_pa(PHYS_TO_VIRT(child->context.pgd),0x4);
        puts("child exit el1\r\n");
        return;
    }

    trapframe_t* child_tf=child_base+((char*)tf - parent_base);
    child_tf->x0=0;
    // puts("parent tf:");
    // output_trapframe(tf);
    //child_tf->sp_el0=child_base+((char*)tf->sp_el0 - parent_base);
    // puts("child_sp_el0:");
    // put_hex(child_tf->sp_el0);
    // puts("\r\n");
    tf->x0=child->pid;
    unsigned long pgd_copy=child->context.pgd;
    
    child->context=parent->context;
    puts("child context:");
    context_output(&child->context);
    puts("parent context:");
    context_output(&parent->context);
    child->context.pgd=pgd_copy;
    map_pages(PHYS_TO_VIRT(child->context.pgd),0,VIRT_TO_PHYS(child->proc),child->proc_size,USER_ATTR_NORMAL_NOCACHE);
    map_pages(PHYS_TO_VIRT(child->context.pgd),PERIPHERAL_START,PERIPHERAL_START,PERIPHERAL_END-PERIPHERAL_START,USER_ATTR_NORMAL_NOCACHE);
    map_pages(PHYS_TO_VIRT(child->context.pgd),0xffffffffb000,VIRT_TO_PHYS(child->u_stack_allo_base+USTACK_SIZE-0x4000),0x4000,USER_ATTR_NORMAL_NOCACHE);
    puts("child pgd_copy:");
    put_long_hex(pgd_copy);
    puts("\r\n");
    puts("in parent, child id:");
    put_int(child->pid);
    puts("\r\n");
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
    puts("in sys_mbox_call\r\n");
    tf->x1=va_to_pa(PHYS_TO_VIRT(running_ring->context.pgd),tf->x1);
    tf->x0=mbox_call_buf((tf->x0),PHYS_TO_VIRT(tf->x1));
    puts("sys_mbox_call exit\r\n");
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