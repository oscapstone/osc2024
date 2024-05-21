#include "syscall.h"
#include "current.h"
#include "sched.h"
#include "stddef.h"
#include "uart.h"
#include "cpio.h"
#include "irq.h"
#include "malloc.h"
#include "mbox.h"
#include "signal.h"
#include "string.h"

int getpid(trapframe_t *tpf) {
    // save return value as current pid
    tpf->x0 = curr_thread->pid;
    return curr_thread->pid;
}

size_t uartread(trapframe_t *tpf, char buf[], size_t size) {
    int i = 0;
    // reading buffer through async uart
    for (int i = 0; i < size; i++)
        buf[i] = uart_async_getc();
    // return size
    tpf->x0 = i;
    return i;
}

size_t uartwrite(trapframe_t *tpf, const char buf[], size_t size) {
    int i = 0;
    // write through async uart
    for (int i = 0; i < size; i++)
        uart_async_putc(buf[i]);
    // size
    tpf->x0 = i;
    return i;
}

// In this lab, you won’t have to deal with argument passing
int exec(trapframe_t *tpf, const char *name, char *const argv[]) {
    // get filesize according filename
    // cpio parse and return data & filesize
    curr_thread->datasize = get_file_size((char *)name);
    char *new_data = get_file_start((char *)name);

    // copy data to replace current thread data
    memcpy(curr_thread->data, new_data, curr_thread->datasize); 

    // clear signal handler
    for (int i = 0; i <= SIGNAL_MAX; i++)
        curr_thread->signal_handler[i] = signal_default_handler;

    // set program (exception return address)
    tpf->elr_el1 = (unsigned long)curr_thread->data;
    // set user stack pointer (el0)
    // kind of reset user stack
    tpf->sp_el0 = (unsigned long)curr_thread->user_sp + USTACK_SIZE;
    tpf->x0 = 0;
    return 0;
}

int fork(trapframe_t *tpf) {
    lock();
    // fork process accrording current program
    // set lr to curr data
    thread_t *child_thread = thread_create(curr_thread->data);

    // copy signal handler
    for (int i = 0; i <= SIGNAL_MAX; i++)
        child_thread->signal_handler[i] = curr_thread->signal_handler[i];

    // store pid
    int parent_pid = curr_thread->pid;
    // warning !!!
    // memcpy before memorize parent thread may pollute stack
    // gdb observed
    thread_t *parent_thread = curr_thread;

    // copy data
    // demo
    child_thread->data = malloc(curr_thread->datasize);
    child_thread->datasize = curr_thread->datasize;
    memcpy(child_thread->data, curr_thread->data, curr_thread->datasize);

    // copy user stack into new process
    memcpy(child_thread->user_sp, curr_thread->user_sp, USTACK_SIZE);

    // copy kernel stack into new process
    memcpy(child_thread->kernel_sp, curr_thread->kernel_sp, KSTACK_SIZE);

    // set child lr
    // we need to set child context below 
    // store current_ctx to  curr_thread
    store_context(current_ctx); 
    // for child
    if (parent_pid != curr_thread->pid)
        goto child;

    // copy parent register
    // callee saved register
    child_thread->context.x19 = curr_thread->context.x19;
    child_thread->context.x20 = curr_thread->context.x20;
    child_thread->context.x21 = curr_thread->context.x21;
    child_thread->context.x22 = curr_thread->context.x22;
    child_thread->context.x23 = curr_thread->context.x23;
    child_thread->context.x24 = curr_thread->context.x24;
    child_thread->context.x25 = curr_thread->context.x25;
    child_thread->context.x26 = curr_thread->context.x26;
    child_thread->context.x27 = curr_thread->context.x28;
    child_thread->context.x28 = curr_thread->context.x28;
    // move fp
    // curr_fp + child_kernel_sp - curr_kernel_sp
    // move relative location
    child_thread->context.fp =  child_thread->kernel_sp + curr_thread->context.fp - curr_thread->kernel_sp; 
    child_thread->context.lr = curr_thread->context.lr;
    // move kernel sp
    // curr_sp + child_kernel_sp - curr_kernel_sp
    // move relative location
    child_thread->context.sp =  child_thread->kernel_sp + curr_thread->context.sp - curr_thread->kernel_sp; 

    unlock();
    // return pid
    tpf->x0 = child_thread->pid;
    return child_thread->pid;

child:
    // move trapframe
    // now is parent tpf
    // tpf : kernel stack
    // offset : tpf - parent->kernel_sp
    // offset + child->kernel_sp is child trapframe
    tpf = (trapframe_t *)((unsigned long)child_thread->kernel_sp + (char *)tpf - (unsigned long)parent_thread->kernel_sp); 
    tpf->sp_el0 = child_thread->user_sp + tpf->sp_el0 - parent_thread->user_sp;
    // child process return 0
    tpf->x0 = 0;
    return 0;
}

void exit(trapframe_t *tpf, int status) {
    thread_exit();
}

int syscall_mbox_call(trapframe_t *tpf, unsigned char ch, unsigned int *mbox) {
    // same as mbox_call 
    // remeber to pass return value by x0
    lock();
    // ch & 0xf setting channel
    // &~0xF send setting of mbox
    // we use 8 (cpu -> gpu)
    unsigned long r = (((unsigned long)((unsigned long)mbox) & ~0xF) | (ch & 0xF));
    /* wait until we can write to the mailbox */
    do {
        asm volatile("nop");
    } while (*MBOX_STATUS & MBOX_FULL);
    /* write the address of our message to the mailbox with channel identifier */
    *MBOX_WRITE = r;
    /* now wait for the response */
    while (1) {
        /* is there a response? */
        do {
            asm volatile("nop");
        } while (*MBOX_STATUS & MBOX_EMPTY);
        /* is it a response to our message? */
        if (r == *MBOX_READ) {
            /* is it a valid successful response? */
            // return value pass by x0
            tpf->x0 = (mbox[1] == MBOX_RESPONSE);
            unlock();
            return mbox[1] == MBOX_RESPONSE;
        }
    }
    // not a respone to our message
    tpf->x0 = 0;
    unlock();
    return 0;
}

void kill(trapframe_t *tpf, int pid) {
    lock();
    // already killed process
    if (pid >= PIDMAX || pid < 0 || threads[pid].status == FREE) {
        unlock();
        return;
    }
    // set dead and move to next 
    // same as thread exit
    threads[pid].status = DEAD;
    unlock();
    schedule();
}

void signal_register(int signal, void (*handler)()) {
    // valid signal
    if (signal > SIGNAL_MAX || signal < 0)
        return;
    // register signal index as given handler
    lock();
    curr_thread->signal_handler[signal] = handler;
    unlock();
}

void signal_kill(int pid, int signal) {
    // check is valid pid to kill
    if (pid >= PIDMAX || pid < 0 || threads[pid].status == FREE)
        return;

    lock();
    // check signal
    // run in kernel -> default handler -> signal kill
    
    // normally only signal index 9 is kill
    threads[pid].sigcount[signal]++;
    unlock();
}

void sigreturn(trapframe_t *tpf) {
    // move stack pointer to the end of stack, then free
    unsigned long signal_ustack = tpf->sp_el0 % USTACK_SIZE == 0 ? 
        tpf->sp_el0 - USTACK_SIZE : tpf->sp_el0 & (~(USTACK_SIZE - 1));
    // recycle handler stack
    free((char *)signal_ustack);
    // restore original saved context
    load_context(&curr_thread->signal_saved_context);
}
