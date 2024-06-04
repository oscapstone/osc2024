#include "bcm2837/rpi_mbox.h"
#include "syscall.h"
#include "sched.h"
#include "uart1.h"
#include "utils.h"
#include "exception.h"
#include "mbox.h"

#include "cpio.h"

extern thread_t *curr_thread;
extern void* CPIO_DEFAULT_START;
extern thread_t threads[PIDMAX + 1];

// trap is like a shared buffer for user space and kernel space
// Because general-purpose registers are used for both arguments and return value,
// We may receive the arguments we need, and overwrite them with return value.

int getpid(trapframe_t *tpf)
{
    tpf->x0 = curr_thread->pid;
    return curr_thread->pid;
}

size_t uartread(trapframe_t *tpf, char buf[], size_t size)
{
    int i = 0;
    for (int i = 0; i < size;i++)
    {
        buf[i] = uart_async_getc();
    }
    tpf->x0 = i+1;
    return i+1;
}

size_t uartwrite(trapframe_t *tpf, const char buf[], size_t size)
{
    int i = 0;
    for (int i = 0; i < size; i++)
    {
        uart_async_putc(buf[i]);
    }
    tpf->x0 = i+1;
    return i+1;
}

//In this lab, you won’t have to deal with argument passing
int exec(trapframe_t *tpf, const char *name, char *const argv[])
{
    curr_thread->datasize = get_file_size((char*)name);
    char *file_start = get_file_start((char *)name);
    for (unsigned int i = 0; i < curr_thread->datasize;i++)
    {
        curr_thread->data[i] = file_start[i];
    }

    tpf->elr_el1 = (unsigned long) curr_thread->data;
    tpf->sp_el0 = (unsigned long) curr_thread->stack_allocated_base + USTACK_SIZE;
    tpf->x0 = 0;
    return 0;
}

int fork(trapframe_t *tpf)
{
    lock();
    thread_t *parent_thread = curr_thread;
    int parent_pid = parent_thread->pid;

    thread_t *child_thread = thread_create(parent_thread->data);

    //copy user stack into new process
    for (int i = 0; i < USTACK_SIZE; i++)
    {
        child_thread->stack_allocated_base[i] = parent_thread->stack_allocated_base[i];
    }

    //copy stack into new process
    for (int i = 0; i < KSTACK_SIZE; i++)
    {
        child_thread->kernel_stack_allocated_base[i] = parent_thread->kernel_stack_allocated_base[i];
    }

    //copy datasize into new process
    child_thread->datasize = parent_thread->datasize;

    store_context(get_current());

    //for child
    if( parent_pid != curr_thread->pid)
    {
        goto child;
    }
    //copy context into new process
    child_thread->context = parent_thread->context;
    // the offset of current syscall should also be updated to new cpu context
    child_thread->context.sp += child_thread->kernel_stack_allocated_base - parent_thread->kernel_stack_allocated_base;
    child_thread->context.fp = child_thread->context.sp;
    unlock();
    tpf->x0 = child_thread->pid;
    // schedule();
    return child_thread->pid;   // pid = new

child:
    // the offset of current syscall should also be updated to new return point
    tpf = (trapframe_t*)((char *)tpf + (unsigned long)child_thread->kernel_stack_allocated_base - (unsigned long)parent_thread->kernel_stack_allocated_base); // move tpf
    tpf->sp_el0 += child_thread->kernel_stack_allocated_base - parent_thread->kernel_stack_allocated_base;
    tpf->x0 = 0;
    // schedule();
    return 0;           // pid = 0
}

void exit(trapframe_t *tpf)
{
    thread_exit();
}

int syscall_mbox_call(trapframe_t *tpf, unsigned char ch, unsigned int *mbox)
{
    lock();
    // Add channel to mbox lower 4 bit
    unsigned long r = (((unsigned long)((unsigned long)mbox) & ~0xF) | (ch & 0xF));
    do{asm volatile("nop");} while (*MBOX_STATUS & BCM_ARM_VC_MS_FULL);
    // Write to Register
    *MBOX_WRITE = r;
    while (1)
    {
        do{ asm volatile("nop");} while (*MBOX_STATUS & BCM_ARM_VC_MS_EMPTY);
        // Read from Register
        if (r == *MBOX_READ)
        {
            tpf->x0 = (mbox[1] == MBOX_REQUEST_SUCCEED);
            unlock();
            return mbox[1] == MBOX_REQUEST_SUCCEED;
        }
    }
    tpf->x0 = 0;
    unlock();
    return 0;
}

void kill(trapframe_t *tpf, int pid)
{
    if ( pid < 0 || pid >= PIDMAX || !threads[pid].isused) return;
    lock();
    threads[pid].iszombie = 1;
    unlock();
    schedule();
}


char* get_file_start(char *thefilepath)
{
    char *filepath;
    unsigned int filesize;
    char *filedata;
    struct cpio_newc_header *header_pointer = CPIO_DEFAULT_START;

    while (header_pointer != 0)
    {
        int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
        //if parse header error
        if (error)
        {
            uart_puts("error");
            break;
        }

        if (strcmp(thefilepath, filepath) == 0)
        {
            return filedata;
        }

        //if this is TRAILER!!! (last of file)
        if (header_pointer == 0)
            uart_puts("execfile: %s: No such file or directory\r\n", thefilepath);
    }
    return 0;
}

unsigned int get_file_size(char *thefilepath)
{
    char *filepath;
    char *filedata;
    unsigned int filesize;
    struct cpio_newc_header *header_pointer = CPIO_DEFAULT_START;

    while (header_pointer != 0)
    {
        int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
        //if parse header error
        if (error)
        {
            uart_puts("error");
            break;
        }

        if (strcmp(thefilepath, filepath) == 0)
        {
            return filesize;
        }

        //if this is TRAILER!!! (last of file)
        if (header_pointer == 0)
            uart_puts("execfile: %s: No such file or directory\r\n", thefilepath);
    }
    return 0;
}
