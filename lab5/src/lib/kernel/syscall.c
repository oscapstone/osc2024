#include "syscall.h"
#include "cpio.h"
#include "mbox.h"
#include "sched.h"
#include "uart.h"

extern char *cpio_start;
extern thread_t *current;
extern thread_t threads[PIDMAX + 1];

int getpid(trapframe_t *tf)
{
    tf->regs[0] = current->pid;
    return current->pid;
}

unsigned int uart_read(trapframe_t *tf, char buf[], unsigned int size)
{
    unsigned int i;
    for (i = 0; i < size; i++) {
        buf[i] = uart_getc();
        // buf[i] = uart_async_getc();
    }
    tf->regs[0] = i;
    return i;
}

unsigned int uart_write(trapframe_t *tf, const char buf[], unsigned int size)
{
    unsigned int i;
    for (i = 0; i < size; i++) {
        uart_send(buf[i]);
        // uart_async_putc(buf[i]);
    }
    tf->regs[0] = i;
    return i;
}

int exec(trapframe_t *tf, const char *name, char *const argv[])
{
    current->datasize = cpio_file_size((char *)name);
    char *new_data = cpio_file_start((char *)name);
    for (unsigned int i = 0; i < current->datasize; i++)
        current->data[i] = new_data[i];

    tf->regs[0] = 0;
    tf->elr_el1 = (unsigned long)current->data;
    tf->sp_el0 = (unsigned long)current->stack_ptr + USTACK_SIZE;
    return 0;
}

int fork(trapframe_t *tf)
{
    lock();
    thread_t *child = thread_create(current->data, NORMAL_PRIORITY);
    thread_t *parent = current;

    child->datasize = current->datasize;

    // copy user stack to child
    for (int i = 0; i < USTACK_SIZE; i++) {
        child->stack_ptr[i] = parent->stack_ptr[i];
    }

    // copy kernel stack to child
    for (int i = 0; i < KSTACK_SIZE; i++) {
        child->kstack_ptr[i] = parent->kstack_ptr[i];
    }

    store_context(get_current()); // save parent context

    if (parent->pid != current->pid) {
        goto fork_child;
    }

    // parent process
    // uart_printf("This is parent process (fork: parent pid = %d, child pid = %d)\n", parent->pid, child->pid);
    child->cpu_context = current->cpu_context; // copy parent cpu context to child
    child->cpu_context.fp += (unsigned long)child->kstack_ptr - (unsigned long)parent->kstack_ptr;
    child->cpu_context.sp += (unsigned long)child->kstack_ptr - (unsigned long)parent->kstack_ptr;

    unlock();
    tf->regs[0] = child->pid;
    return child->pid;

fork_child: // child process
    // uart_printf("This is child process (fork: parent pid = %d, child pid = %d)\n", parent->pid, child->pid);

    tf = (trapframe_t *)((char *)tf + (unsigned long)child->kstack_ptr - (unsigned long)parent->kstack_ptr);
    tf->sp_el0 += (unsigned long)child->stack_ptr - (unsigned long)parent->stack_ptr;
    tf->regs[0] = 0;
    return 0;
}

void exit(trapframe_t *tf) { thread_exit(); }

int mbox_call(trapframe_t *tf, unsigned char ch, unsigned int *mbox)
{
    lock();
    unsigned long r = (((unsigned long)((unsigned long)mbox) & ~0xF) | (ch & 0xF));

    while (*MAILBOX_STATUS & MAILBOX_FULL) {
        asm volatile("nop");
    }

    *MAILBOX_WRITE = r;

    while (1) {
        while (*MAILBOX_STATUS & MAILBOX_EMPTY) {
            asm volatile("nop");
            uart_printf("mailbox : %x ", *MAILBOX_STATUS);
            uart_printf("mailbox empty : %x\n", MAILBOX_EMPTY);
            *MAILBOX_STATUS = 0;
            break;
        }

        if (r == *MAILBOX_READ) {
            tf->regs[0] = (mbox[1] == REQUEST_SUCCEED);
            unlock();
            return mbox[1] == REQUEST_SUCCEED;
        }
    }

    tf->regs[0] = 0;
    unlock();
    return 0;
}

void kill(trapframe_t *tf, int pid)
{
    if (pid < 0 || pid >= PIDMAX || !threads[pid].used)
        return;

    lock();
    threads[pid].zombie = 1;
    unlock();
    schedule();
}

char *cpio_file_start(char *file)
{
    char *current_filename;
    void *filedata;
    unsigned int size;
    struct cpio_newc_header *header = (struct cpio_newc_header *)(cpio_start);

    while (header) {
        int error = cpio_parse_header(header, &current_filename, &size, &filedata, &header);
        if (error) {
            uart_puts("No such file or directory\n");
            break;
        }
        if (!strcmp(current_filename, file)) {
            return filedata;
        }
    }
    return 0;
}

unsigned int cpio_file_size(char *file)
{
    char *current_filename;
    void *filedata;
    unsigned int size;
    struct cpio_newc_header *header = (struct cpio_newc_header *)(cpio_start);

    while (header) {
        int error = cpio_parse_header(header, &current_filename, &size, &filedata, &header);
        if (error) {
            uart_puts("No such file or directory\n");
            break;
        }
        if (!strcmp(current_filename, file)) {
            return size;
        }
    }
    return 0;
}