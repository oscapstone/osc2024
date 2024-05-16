#include "bcm2837/rpi_mbox.h"
#include "syscall.h"
#include "schedule.h"
#include "uart.h"
#include "string.h"
#include "exception.h"
#include "memory.h"
#include "mbox.h"
#include "signal.h"

#include "cpio.h"
#include "dtb.h"

extern void *CPIO_START;
extern thread_t *curr_thread;
extern thread_t threads[PIDMAX + 1];

//  Get current process’s id.
int getpid(trapframe_t *tp)
{
    tp->x0 = curr_thread->pid;
    return curr_thread->pid;
}

//  Return the number of bytes read by reading size byte into the user-supplied buffer buf.
size_t uartread(trapframe_t *tp, char buf[], size_t size)
{
    int i;
    for (i = 0; i < size; i++)
    {
        buf[i] = uart_async_getc();
    }
    tp->x0 = i;
    return i;
}

//  Return the number of bytes written after writing size byte from the user-supplied buffer buf.
size_t uartwrite(trapframe_t *tp,const char buf[], size_t size)
{
    int i;
    for (i = 0; i < size; i++)
    {
        uart_async_putc(buf[i]);
    }
    tp->x0 = i;
    return i;
}

//  Run the program with parameters.
//  In this lab, you won’t have to deal with argument passing, but you can still use it.
int exec(trapframe_t *tp, const char *name, char *const argv[])
{
    curr_thread->datasize = get_file_size((char *)name);
    char *new_data = get_file_start((char *)name);
    // store data
    for (unsigned int i = 0; i < curr_thread->datasize; i++)
    {
        curr_thread->data[i] = new_data[i];
    }

    // initial signal handler
    for (int i = 0; i <= SIGNAL_MAX; i++)
    {
        curr_thread->signal_handler[i] = signal_default_handler;
    }

    tp->elr_el1 = (unsigned long)curr_thread->data;                           // 要執行的program
    tp->sp_el0 = (unsigned long)curr_thread->stack_allocted_ptr + USTACK_SIZE; // sp of the program
    tp->x0 = 0;
    return 0;
}

//  The standard method of duplicating the current process in UNIX-like operating systems is to use fork().
//  Following the call to fork(), two processes run the same code.
//  Set the parent process’s return value to the child’s id and the child process’s return value to 0 to distinguish them.
int fork(trapframe_t *tp)
{
    disable_irq();
    thread_t *new_thread = thread_create(curr_thread->data); // copy of parent thread, same code, data with parent
    new_thread->datasize = curr_thread->datasize;

    // copy handler
    for (int i = 0; i <= SIGNAL_MAX; i++)
    {
        new_thread->signal_handler[i] = curr_thread->signal_handler[i];
    }

    int ppid = curr_thread->pid;
    thread_t *pthread = curr_thread;

    // copy user stack
    for (int i = 0; i < USTACK_SIZE; i++)
    {
        new_thread->stack_allocted_ptr[i] = pthread->stack_allocted_ptr[i];
    }
    // copy kernel stack
    for (int i = 0; i < KSTACK_SIZE; i++)
    {
        new_thread->kernel_stack_allocted_ptr[i] = pthread->kernel_stack_allocted_ptr[i];
    }
    // copy register
    store_context(get_current());

    // in child thread
    if (ppid != curr_thread->pid)
    {
        // the offset of current syscall should also be updated to new return point
        tp = (trapframe_t *)((char *)tp + (unsigned long)new_thread->kernel_stack_allocted_ptr - (unsigned long)pthread->kernel_stack_allocted_ptr); // move tp
        tp->sp_el0 += new_thread->stack_allocted_ptr - pthread->stack_allocted_ptr;
        tp->x0 = 0;
        return 0;
    }

    new_thread->context = curr_thread->context;
    // add the offset to kernel stack
    long unsigned int offset = new_thread->kernel_stack_allocted_ptr - curr_thread->kernel_stack_allocted_ptr; 
    new_thread->context.fp += offset; // start of stack
    new_thread->context.sp += offset; // stack pointer

    
    enable_irq();
    tp->x0 = new_thread->pid;
    return new_thread->pid;
}

//  Terminate the current process.
void exit(trapframe_t *tp, int status)
{
    thread_exit();
}

//  Get the hardware’s information by mailbox
int syscall_mbox_call(trapframe_t *tp, unsigned char ch, unsigned int *mbox)
{
    disable_irq();
    unsigned long value = (((unsigned long)((unsigned long)mbox) & ~0xF) | (ch & 0xF));
    while (*MBOX_STATUS & BCM_ARM_VC_MS_FULL)
        ;
    *MBOX_WRITE = value;
    while (1)
    {
        while (*MBOX_STATUS & BCM_ARM_VC_MS_EMPTY)
            ;
        if (value == *MBOX_READ)
        {
            tp->x0 = (mbox[1] == MBOX_REQUEST_SUCCEED);
            enable_irq();
            return mbox[1] == MBOX_REQUEST_SUCCEED;
        }
    }
    tp->x0 = 0;
    enable_irq();
    return 0;
}

//  Other processes identified by pid should be terminated.
//  You don’t need to implement this system call if you prefer to kill a process using the POSIX Signal stated in Advanced Exercise 1.
void kill(trapframe_t *tp, int pid)
{
    if (pid < 0 || pid >= PIDMAX || !threads[pid].isused)
        return;
    disable_irq();
    threads[pid].iszombie = 1;
    enable_irq();
    schedule();
}

// system call number: 8
// register signal handler
void register_signal(int signal, void (*handler)())
{
    if (signal > SIGNAL_MAX || signal < 0)
        return;
    curr_thread->signal_handler[signal] = handler;
}

// system call number: 9
// kill by signal
void signal_kill(int pid, int signal)
{
    if (pid > PIDMAX || pid < 0 || !threads[pid].isused)
        return;
    disable_irq();
    threads[pid].sigcount[signal]++;
    enable_irq();
}

void sigreturn(trapframe_t *tp)
{
    unsigned long signal_ustack = (tp->sp_el0 % USTACK_SIZE == 0) ? tp->sp_el0 - USTACK_SIZE : tp->sp_el0 & (~(USTACK_SIZE - 1));
    kfree((char*)signal_ustack);
    load_context(&curr_thread->signal_savedContext);
}

// get file data
char *get_file_start(char *thefilepath)
{
    char *filepath;
    char *filedata;
    unsigned int filesize;
    struct cpio_newc_header *header_pointer = CPIO_START;

    while (header_pointer != 0)
    {
        // if parse header error
        if (cpio_newc_parse(header_pointer, &filepath, &filesize, &filedata, &header_pointer))
        {
            uart_puts("error");
            break;
        }

        if (strcmp(thefilepath, filepath) == 0)
        {
            return filedata;
        }

        // if this is TRAILER!!! (last of file)
        if (header_pointer == 0)
            uart_puts("execfile: %s: No such file or directory\r\n", thefilepath);
    }
    return 0;
}

// get file size
unsigned int get_file_size(char *thefilepath)
{
    char *filepath;
    char *filedata;
    unsigned int filesize;
    struct cpio_newc_header *header_pointer = CPIO_START;

    while (header_pointer != 0)
    {
        // if parse header error
        if (cpio_newc_parse(header_pointer, &filepath, &filesize, &filedata, &header_pointer))
        {
            uart_puts("error");
            break;
        }

        if (strcmp(thefilepath, filepath) == 0)
        {
            return filesize;
        }

        // if this is TRAILER!!! (last of file)
        if (header_pointer == 0)
            uart_puts("execfile: %s: No such file or directory\r\n", thefilepath);
    }
    return 0;
}