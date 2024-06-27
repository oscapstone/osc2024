#include "syscall.h"
#include "initrd.h"
#include "alloc.h"
#include "string.h"
#include "exception.h"
#include "mbox.h"
#include "signal.h"
#include "mmu.h"
#include <stdint.h>

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

static inline void copy_regs(callee_reg_t *regs)
{
    uart_send_string("copy_regs\n");
    uart_hex(get_current_thread()->tid);
    uart_send_string("\n");
    regs->x19 = get_current_thread()->callee_reg.x19;
    regs->x20 = get_current_thread()->callee_reg.x20;
    regs->x21 = get_current_thread()->callee_reg.x21;
    regs->x22 = get_current_thread()->callee_reg.x22;
    regs->x23 = get_current_thread()->callee_reg.x23;
    regs->x24 = get_current_thread()->callee_reg.x24;
    regs->x25 = get_current_thread()->callee_reg.x25;
    regs->x26 = get_current_thread()->callee_reg.x26;
    regs->x27 = get_current_thread()->callee_reg.x27;
    regs->x28 = get_current_thread()->callee_reg.x28;
    regs->fp = get_current_thread()->callee_reg.fp;
    regs->lr = get_current_thread()->callee_reg.lr;
    regs->sp = get_current_thread()->callee_reg.sp;
}

int exec(const char* name, char *const argv[]) {
    char* target_addr;
    char *filepath;
    char *filedata;
    unsigned int filesize;
    // current pointer
    thread_t* cur_thread = get_current_thread();
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
            uart_send_string("filesize: ");
            uart_hex(filesize);
            uart_send_string("\n");
            cur_thread -> data = (void*)kmalloc(filesize);
            cur_thread -> data_size = filesize;
            uart_send_string("Copying user program\n");
            memcpy(cur_thread -> data, filedata, cur_thread -> data_size);
            uart_send_string("Finished\n");
            break;
        }

        // if this is not TRAILER!!! (last of file)
        if (header_pointer == 0){
            uart_send_string("Program not found\n");
            return 1;
        }
    }
    
    // TODO: signal handling, should clear all signal handler here
    for(int i=0;i<=SIGNAL_NUM;i++) {
        cur_thread -> signal_handler[i] = 0;
        cur_thread -> waiting_signal[i] = 0;
    }
    unsigned long kernel_sp = (unsigned long)cur_thread -> kernel_stack + T_STACK_SIZE;

    // cur_thread -> callee_reg.lr = (unsigned long)target_addr;
    cur_thread -> page_table = pt_create();
    pt_map(cur_thread -> page_table, (void*)0, cur_thread -> data_size, 
           (void*)VA2PA(cur_thread -> data), PT_R | PT_W | PT_X); // map user program
    pt_map(cur_thread -> page_table, (void*)0xffffffffb000, T_STACK_SIZE,
        (void*)VA2PA(cur_thread -> user_stack), PT_R | PT_W); // map user stack

    pt_map(cur_thread->page_table, (void *)0x3c000000, 0x04000000,
            (void *)0x3c000000, PT_R | PT_W); // map mailbox
    
    set_page_table(cur_thread);
    exec_user_prog((void *)0, (char *)0xffffffffeff0, kernel_sp);

    return 0;
}

void fork(trapframe_t* tf) {
    // uart_send_string("forked\n");
    thread_t* parent_thread = get_current_thread();
    thread_t* child_thread = create_fork_thread();

    child_thread -> data = (void*)kmalloc(parent_thread -> data_size);
    child_thread -> data_size = parent_thread -> data_size;

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

    memcpy(
        (void*)child_thread->data,
        (void*)parent_thread->data,
        (uint64_t)parent_thread->data_size
    );

    child_thread -> page_table = pt_create();

    pt_map(child_thread->page_table, (void *)0, child_thread->data_size,
           (void *)VA2PA(child_thread->data), PT_R | PT_W | PT_X);
    pt_map(child_thread->page_table, (void *)0xffffffffb000, T_STACK_SIZE,
           (void *)VA2PA(child_thread->user_stack), PT_R | PT_W);

    // TODO: Why is this needed for the vm.img to run?
    pt_map(child_thread->page_table, (void *)0x3c000000, 0x04000000,
           (void *)0x3c000000, PT_R | PT_W);

    save_regs(parent_thread);
    copy_regs(&child_thread->callee_reg);

    // copy signal_handler
    for(int i=0;i<=SIGNAL_NUM;i++) {
        child_thread -> signal_handler[i] = parent_thread -> signal_handler[i];
        child_thread -> waiting_signal[i] = parent_thread -> waiting_signal[i];
    }

    uint64_t parent_sp = get_current_thread() -> callee_reg.sp;
    uint64_t parent_fp = get_current_thread() -> callee_reg.fp;


    child_thread -> callee_reg.sp = (uint64_t)((uint64_t)parent_sp - (uint64_t)parent_thread->kernel_stack + (uint64_t)child_thread->kernel_stack);
    child_thread -> callee_reg.fp = (uint64_t)((uint64_t)parent_fp - (uint64_t)parent_thread->kernel_stack + (uint64_t)child_thread->kernel_stack);

    trapframe_t *child_tf = (trapframe_t*)(child_thread -> kernel_stack +
        ((char*)tf - (char*)(parent_thread -> kernel_stack))
    );

    void* label_address = &&SYSCALL_FORK_END;
    uart_send_string("Address of SYSCALL_FORK_END: ");
    uart_hex((uint64_t)label_address);
    uart_send_string("\n");

    child_thread -> callee_reg.lr = &&SYSCALL_FORK_END;

    // set child trapframe
   
    // set child user stack sp
    child_tf -> x[0] = 0;
    child_tf -> x[30] = tf -> x[30];
    child_tf -> sp_el0 = tf -> sp_el0;
    child_tf -> spsr_el1 = tf -> spsr_el1;
    child_tf -> elr_el1 = tf -> elr_el1;
    uart_send_string("elr_el1: ");
    uart_hex(child_tf -> elr_el1);
    uart_send_string("\n");
    tf -> x[0] = child_thread -> tid;
    push_running(child_thread);
SYSCALL_FORK_END:
    uart_send_string("forked end\n");
    uart_send_string("current tid: ");
    uart_hex(get_current_thread()->tid);
    uart_send_string("\n");
    asm volatile("nop");
    return;
}

void exit(int status) {
    thread_exit();
}

int mbox_call(unsigned char ch, unsigned int *mbox) {
    int mbox_size;
    char *kmbox;

    mbox_size = (int)mbox[0];

    if (mbox_size <= 0)
        return;

    kmbox = kmalloc(mbox_size);

    memcpy(kmbox, (char *)mbox, mbox_size);

    mailbox_call(ch, (unsigned int *)kmbox);

    memcpy((char *)mbox, kmbox, mbox_size);

    kfree(kmbox);
}

void kill(int pid) {
    kill_thread(pid);
}

void signal(int signal, void (*handler)()) {
    thread_t* t = get_current_thread();
    uart_send_string("register signal\n");
    uart_hex(t->tid);
    uart_send_string("\n");
    uart_send_string("singal num: ");
    uart_hex(signal);
    uart_send_string("\n");
    t -> signal_handler[signal] = handler;
}

void posix_kill(int pid, int signal) {
    thread_t* t = get_thread_from_tid(pid);
    if(!t) return;
    uart_send_string("[set signal] tid: ");
    uart_hex(t->tid);
    uart_send_string("\n");
    t -> waiting_signal[signal] = 1;
}

void sigreturn() {
    thread_t* t = get_current_thread();
    // uart_send_string("sigreturn\n tid: ");
    // uart_hex(t->tid);
    // uart_send_string("\n");
    kfree(t -> callee_reg.fp - T_STACK_SIZE);
    load_regs(t->signal_regs);
    t -> is_processing_signal = 0;
    return;
}


int sys_getpid() {
    int res;
    asm volatile("mov x8, 0");
    asm volatile("svc 0");
    asm volatile("mov %0, x0": "=r"(res));
    return res;
}

int sys_fork() {
    int res;
    asm volatile("mov x8, 4");
    asm volatile("svc 0");
    asm volatile("mov %0, x0": "=r"(res));
    return res;
}

void sys_exit(int status){
    asm volatile("mov x8, 5");
    asm volatile("svc 0");
}

void sys_posix_kill(){
    asm volatile("mov x8, 9");
    asm volatile("svc 0");
}

void sys_sigreturn() {
    asm volatile("mov x8, 20");
    asm volatile("svc 0");
}