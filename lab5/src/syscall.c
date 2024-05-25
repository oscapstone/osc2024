#include "syscall.h"
#include "initrd.h"
#include "alloc.h"
#include "string.h"
#include "exception.h"
#include "mbox.h"

int getpid() {
    thread_t* t = get_current_thread();
    return t -> tid;
}

size_t uart_read(char buf[], size_t size) {
    size_t i;
    for(i=0;i<size;i++){
        buf[i] = uart_recv();
    }
    return i;
}
size_t uart_write(const char buf[], size_t size) {
    size_t i;
    for(i=0;i<size;i++){
        uart_send(buf[i]);
    }
    return i;
}

int exec(const char* name, char *const argv[]) {
    char* target_addr;
    char *filepath;
    char *filedata;
    unsigned int filesize;
    // current pointer
    cpio_t *header_pointer = (cpio_t *)(ramfs_base);

    // print every cpio pathname
    while (header_pointer)
    {

        int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
        // if parse header error
        if (error)
        {
            // uart_printf("error\n");
            uart_send_string("Error parsing cpio header\n");
            break;
        }
        if (!strcmp(name, filepath))
        {  
            target_addr = (char*)kmalloc(filesize);
            memcpy(target_addr, filedata, filesize);
            break;
        }

        // if this is not TRAILER!!! (last of file)
        if (header_pointer == 0){
            uart_send_string("Program not found\n");
            return 1;
        }
    }
    // run program from current thread
    thread_t* cur_thread = get_current_thread();
    
    // TODO: signal handling, should clear all signal handler here

    cur_thread -> callee_reg.lr = (unsigned long)target_addr;

    unsigned long spsr_el1 = 0x0; // run in el0 and enable all interrupt (DAIF)
    unsigned long elr_el1 = cur_thread -> callee_reg.lr;
    unsigned long user_sp = cur_thread -> callee_reg.sp;
    unsigned long kernel_sp = (unsigned long)cur_thread -> kernel_stack + T_STACK_SIZE;

    // "r": Any general-purpose register, except sp
    asm volatile("msr tpidr_el1, %0" : : "r" (cur_thread));
    asm volatile("msr spsr_el1, %0" : : "r" (spsr_el1));
    asm volatile("msr elr_el1, %0" : : "r" (elr_el1));
    asm volatile("msr sp_el0, %0" : : "r" (user_sp));
    asm volatile("mov sp, %0" :: "r" (kernel_sp));
    asm volatile("eret"); // jump to user program
    return 0;
}

int fork(trapframe_t* tf) {
    
    thread_t* parent_thread = get_current_thread();
    thread_t* child_thread = create_fork_thread(0);
    

    uart_send_string("parent: ");
    uart_hex(parent_thread->user_stack);
    uart_send_string(", child: ");
    uart_hex(child_thread->user_stack);
    uart_send_string("\n");
    // copy user stack
    memcpy(
        (void*)child_thread->user_stack,
        (void*)parent_thread->user_stack,
        T_STACK_SIZE
    );

    // copy kernel stack
    memcpy(
        (void*)child_thread->kernel_stack,
        (void*)parent_thread->kernel_stack,
        T_STACK_SIZE
    );

    // TODO: copy signal handler

    unsigned long parent_sp;
    asm volatile("mov %0, sp": "=r"(parent_sp));
    void* offset_kernel_stack = child_thread->kernel_stack - parent_thread->kernel_stack;

    child_thread -> callee_reg.sp = (unsigned long)(parent_sp + offset_kernel_stack);
    // set child trapframe
    trapframe_t *child_tf = (trapframe_t*)(child_thread -> kernel_stack +
        ((char*)tf - (char*)(parent_thread -> kernel_stack))
    );

    // set child user stack sp
    child_tf->x[0] = 0;
    child_tf->sp_el0 = (unsigned long)(child_thread -> user_stack + 
        ((void*)tf->sp_el0 - parent_thread->user_stack)
    );
    child_tf->spsr_el1 = tf->spsr_el1;
    child_tf->elr_el1 = tf->elr_el1;
    
    // set child lr
    unsigned long lr;
    asm volatile("mov %0, lr" : "=r" (lr));

    // uart_send_string("[INFO] child lr: ");
    // uart_hex(lr);
    // uart_send_string("\n");

    child_thread -> callee_reg.lr = lr;



    if(get_current_thread() -> tid == child_thread -> tid) return 0;
    // uart_send_string("[INFO] child tid: ");
    // uart_hex(child_thread -> tid);
    // uart_send_string("\n");
    push_running(child_thread);
    tf -> x[0] = child_thread -> tid;
    return child_thread -> tid;
}

void exit(int status) {
    thread_exit();
}

int mbox_call(unsigned char ch, unsigned int *mbox) {
    return mailbox_call(ch, mbox);
}

void kill(int pid) {
    kill_thread(pid);
}


int sys_getpid() {
    int res;
    asm volatile("mov x8, 0");
    asm volatile("svc 0");
    asm volatile("mov %0, x0": "=r"(res));
    return res;
}

int sys_fork() {
    // el1_interrupt_disable();
    int res;
    asm volatile("mov x8, 4");
    asm volatile("svc 0");
    asm volatile("mov %0, x0": "=r"(res));
    // el1_interrupt_enable();
    return res;
}

void sys_exit(int status){
    asm volatile("mov x8, 5");
    asm volatile("svc 0");
}