#include "bcm2837/rpi_mbox.h"
#include "syscall.h"
#include "sched.h"
#include "uart1.h"
#include "u_string.h"
#include "exception.h"
#include "memory.h"
#include "mbox.h"
#include "signal.h"

#include "cpio.h"
#include "dtb.h"

extern void* CPIO_DEFAULT_START;
extern thread_t *curr_thread;
extern thread_t threads[PIDMAX + 1];
extern int uart_recv_echo_flag; // to prevent the syscall.img from using uart_send() in uart_recv()

int syscall_num = 0;
SYSCALL_TABLE_T syscall_table [] = {
    { .func=getpid                  },
    { .func=uartread                },
    { .func=uartwrite               },
    { .func=exec                    },
    { .func=fork                    },
    { .func=exit                    },
    { .func=syscall_mbox_call       },
    { .func=kill                    },
    { .func=signal_register         },
    { .func=signal_kill             },
    { .func=signal_reture           }
};

void init_syscall()
{
    syscall_num = sizeof(syscall_table) / sizeof(SYSCALL_TABLE_T);
    // uart_sendline("syscall_table[syscall_no].func: 0x%x\r\n", syscall_table[0].func);

}
// trap is like a shared buffer for user space and kernel space
// Because general-purpose registers are used for both arguments and return value,
// We may receive the arguments we need, and overwrite them with return value.

SYSCALL_DEFINE(getpid)
{
    // uart_sendline("getpid: %d\r\n", curr_thread->pid);
    tpf->x0 = curr_thread->pid;
    return curr_thread->pid;
}

SYSCALL_DEFINE(uartread)
{
    char *buf = (char *)tpf->x0;
    size_t size = tpf->x1;

    int i = 0;
    for (int i = 0; i < size;i++)
    {
        buf[i] = uart_async_getc();
    }
    tpf->x0 = i;
    return i;
}

SYSCALL_DEFINE(uartwrite)
{
    // uart_sendline("uartwrite: %s\r\n", (char *)tpf->x0);
    const char *buf = (const char *)tpf->x0;
    size_t size = tpf->x1;

    int i = 0;
    for (int i = 0; i < size; i++)
    {
        uart_async_putc(buf[i]);
    }
    tpf->x0 = i;
    return i;
}

//In this lab, you won’t have to deal with argument passing
SYSCALL_DEFINE(exec)
{
    char *name = (char *)tpf->x0;
    // char *argv[] = (char **)tpf->x1;

    curr_thread->datasize = get_file_size((char*)name);
    char *new_data = get_file_start((char *)name);
    for (unsigned int i = 0; i < curr_thread->datasize;i++)
    {
        curr_thread->data[i] = new_data[i];
    }

    // inital signal handler
    for (int i = 0; i <= SIGNAL_MAX; i++)
    {
        curr_thread->signal_handler[i] = signal_default_handler;
    }

    tpf->elr_el1 = (unsigned long) curr_thread->data;
    tpf->sp_el0 = (unsigned long) curr_thread->stack_alloced_ptr + USTACK_SIZE;
    tpf->x0 = 0;
    return 0;
}

SYSCALL_DEFINE(fork)
{
    lock();
    thread_t *newt = thread_create(curr_thread->data, NORMAL_PRIORITY);
    // thread_t *newt = thread_create(curr_thread->data, 10);
    newt->datasize = curr_thread->datasize;

    //copy signal handler
    for (int i = 0; i <= SIGNAL_MAX;i++)
    {
        newt->signal_handler[i] = curr_thread->signal_handler[i];
    }

    int parent_pid = curr_thread->pid;
    thread_t *parent_thread = curr_thread;

    //copy user stack into new process
    for (int i = 0; i < USTACK_SIZE; i++)
    {
        newt->stack_alloced_ptr[i] = curr_thread->stack_alloced_ptr[i];
    }

    //copy kernel stack into new process
    for (int i = 0; i < KSTACK_SIZE; i++)
    {
        newt->kernel_stack_alloced_ptr[i] = curr_thread->kernel_stack_alloced_ptr[i];
    }

    store_context(get_current());

    //for child
    if( parent_pid != curr_thread->pid)
    {
        goto child;
    }

    newt->context = curr_thread->context;
    // the offset of current syscall should also be updated to new cpu context
    newt->context.fp += newt->kernel_stack_alloced_ptr - curr_thread->kernel_stack_alloced_ptr;
    newt->context.sp += newt->kernel_stack_alloced_ptr - curr_thread->kernel_stack_alloced_ptr;

    unlock();
    tpf->x0 = newt->pid;
    return newt->pid;   // pid = new

child:
    // the offset of current syscall should also be updated to new return point
    tpf = (trapframe_t*)((char *)tpf + (unsigned long)newt->kernel_stack_alloced_ptr - (unsigned long)parent_thread->kernel_stack_alloced_ptr); // move tpf
    tpf->sp_el0 += newt->stack_alloced_ptr - parent_thread->stack_alloced_ptr;
    tpf->x0 = 0;
    return 0;           // pid = 0
}

SYSCALL_DEFINE(exit)
{
    // int status = tpf->x0;
    uart_recv_echo_flag = 1;
    thread_exit();
    return 0;
}

SYSCALL_DEFINE(syscall_mbox_call)
{
    unsigned char ch = (unsigned char)tpf->x0;
    unsigned int *mbox = (unsigned int *)tpf->x1;
    lock();
    unsigned long r = (((unsigned long)((unsigned long)mbox) & ~0xF) | (ch & 0xF));
    do{asm volatile("nop");} while (*MBOX_STATUS & BCM_ARM_VC_MS_FULL);
    *MBOX_WRITE = r;
    while (1)
    {
        do{ asm volatile("nop");} while (*MBOX_STATUS & BCM_ARM_VC_MS_EMPTY);
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

SYSCALL_DEFINE(kill)
{
    int pid = tpf->x0;
    if ( pid < 0 || pid >= PIDMAX || !threads[pid].isused) return -1;
    lock();
    threads[pid].iszombie = 1;
    unlock();
    schedule();
    return 0;
}

SYSCALL_DEFINE(signal_register)
{
    int signal = tpf->x0;
    void (*handler)() = (void (*)())tpf->x1;

    if (signal > SIGNAL_MAX || signal < 0) return -1;
    curr_thread->signal_handler[signal] = handler;
    return 0;
}

SYSCALL_DEFINE(signal_kill)
{
    int pid = tpf->x0;
    int signal = tpf->x1;

    if (pid > PIDMAX || pid < 0 || !threads[pid].isused)return -1;
    lock();
    threads[pid].sigcount[signal]++;
    unlock();
    return 0;
}

SYSCALL_DEFINE(signal_reture)
{
    unsigned long signal_ustack = 
        tpf->sp_el0 % USTACK_SIZE == 0 ? 
        tpf->sp_el0 - USTACK_SIZE :             // if the signal doesn't use the stack
        tpf->sp_el0 & (~(USTACK_SIZE - 1));     // if the signal uses the stack, return to the start of the stack 
    kfree((char*)signal_ustack);
    load_context(&curr_thread->signal_saved_context);
    return 0;
}


char* get_file_start(char *thefilepath)
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
