#include "gpio.h"
#include "uart.h"
#include "task.h"
#include "timer.h"
#include "schedule.h"
#include "mailbox.h"
#include "allocator.h"
#include "exception.h"

#define AUX_MU_IO ((volatile unsigned int *)(MMIO_BASE + 0x00215040))
#define AUX_MU_IER ((volatile unsigned int *)(MMIO_BASE + 0x00215044))
#define AUX_MU_IIR ((volatile unsigned int *)(MMIO_BASE + 0x00215048))
#define AUX_MU_LSR ((volatile unsigned int *)(MMIO_BASE + 0x00215054))

#define TX_PRIORITY 4
#define RX_PRIORITY 4
#define TIMER_PRIORITY 0

extern char read_buf[MAX_SIZE];
extern char write_buf[MAX_SIZE];
extern int read_front;
extern int read_back;
extern int write_front;
extern int write_back;

extern task_heap *task_hp;
extern timer_heap *timer_hp;

void enable_interrupt()
{
    asm volatile("msr DAIFClr, 0xf");
}

void disable_interrupt()
{
    asm volatile("msr DAIFSet, 0xf");
}

void exception_entry()
{
    uart_puts("In Exception handle\n");

    unsigned long long spsr_el1 = 0;
    asm volatile("mrs %0, spsr_el1" : "=r"(spsr_el1));
    uart_puts("spsr_el1: ");
    uart_hex_lower_case(spsr_el1);
    uart_puts("\n");

    unsigned long long elr_el1 = 0;
    asm volatile("mrs %0, elr_el1" : "=r"(elr_el1));
    uart_puts("elr_el1: ");
    uart_hex_lower_case(elr_el1);
    uart_puts("\n");

    unsigned long long esr_el1 = 0;
    asm volatile("mrs %0, esr_el1" : "=r"(esr_el1));
    uart_puts("esr_el1: ");
    uart_hex_lower_case(esr_el1);
    uart_puts("\n");

    return;
}

void sync_handler_entry()
{
    uart_puts("Into sync handler\n");
}

void el0_svc_handler_entry(struct ucontext *trapframe)
{
    int get_syscall_no = trapframe->x[8];
    switch (get_syscall_no)
    {
    case SYS_GET_PID:
        sys_getpid(trapframe);
        break;
    case SYS_UART_READ:
        sys_uartread(trapframe);
        break;
    case SYS_UART_WRITE:
        sys_uartwrite(trapframe);
        break;
    case SYS_EXEC:
        sys_exec(trapframe);
        break;
    case SYS_FORK:
        sys_fork(trapframe);
        break;
    case SYS_EXIT:
        sys_exit(trapframe);
        break;
    case SYS_MBOX_CALL:
        sys_mbox_call(trapframe);
        break;
    case SYS_KILL:
        sys_kill(trapframe);
        break;
    case SYS_SIGNAL:
        sys_signal(trapframe);
        break;
    case SYS_SIGNAL_KILL:
        sys_signal_kill(trapframe);
        break;
    case SYS_SIGRETURN:
        sys_sigreturn(trapframe);
        break;

    default:
        break;
    }
}

void irq_handler_entry()
{
    if (*CORE0_INTERRUPT_SOURCE & (1 << 8)) // interrupt is from GPU
    {
        if (*AUX_MU_IIR & 0b100) // check if it's receiver interrupt.
        {
            disable_uart_rx_interrupt(); // masks the device’s interrupt line
            task t;
            t.callback = rx_task;
            t.priority = RX_PRIORITY;

            disable_interrupt();
            task_heap_insert(task_hp, t);
            enable_interrupt();

            do_task();
        }
        else if (*AUX_MU_IIR & 0b010) // check if it's transmiter interrupt
        {
            disable_uart_tx_interrupt(); // masks the device’s interrupt line
            task t;
            t.callback = tx_task;
            t.priority = TX_PRIORITY;

            disable_interrupt();
            task_heap_insert(task_hp, t);
            enable_interrupt();

            do_task();
        }
    }
    else if (*CORE0_INTERRUPT_SOURCE & (1 << 1)) // interrupt is from CNTPNSIRQ
    {
        core_timer_disable(); // masks the device’s interrupt line
        task t;
        t.callback = timer_task;
        t.priority = TIMER_PRIORITY;

        disable_interrupt();
        task_heap_insert(task_hp, t);
        enable_interrupt();

        do_task();
    }
}

void rx_task()
{
    if ((read_back + 1) % MAX_SIZE == read_front) // if buffer is full, then return.
        return;

    while ((*AUX_MU_LSR & 1) && ((read_back + 1) % MAX_SIZE != read_front))
    {
        read_buf[read_back] = uart_read();
        read_back = (read_back + 1) % MAX_SIZE;
    }
}

void tx_task()
{
    if (write_front == write_back) // if buffer is empty, then return.
        return;

    while (write_front != write_back)
    {
        uart_write(write_buf[write_front]);
        write_front = (write_front + 1) % MAX_SIZE;
    }
}

void timer_task()
{
    timer t = timer_heap_extractMin(timer_hp);
    if (t.expire == -1) // the timer heap is empty
        return;

    t.callback(t.data, t.executed_time);

    if (timer_hp->size > 0)
    {
        set_min_expire();
        core_timer_enable();
    }
    else
        core_timer_disable();
}

void sys_getpid(struct ucontext *trapframe)
{
    trapframe->x[0] = get_current_task()->id;
}

void sys_uartread(struct ucontext *trapframe)
{
    enable_interrupt(); // to enable nested interrupts in system call, because the interrupt may be closed when go into synchronous handler?

    char *buf = (char *)trapframe->x[0];
    int size = trapframe->x[1];

    for (int i = 0; i < size; i++)
        buf[i] = uart_read();

    buf[size] = '\0';
    trapframe->x[0] = size;

    disable_interrupt();
}

void sys_uartwrite(struct ucontext *trapframe)
{
    char *buf = (char *)trapframe->x[0];
    int size = trapframe->x[1];

    for (int i = 0; i < size; i++)
        uart_write(buf[i]);

    trapframe->x[0] = size;
}

void sys_exec(struct ucontext *trapframe)
{
    const char *name = (const char *)trapframe->x[0];
    char **const argv = (char **const)trapframe->x[1];
    do_exec(name, argv);
    trapframe->x[0] = -1; // if do_exec is falut
}

void sys_fork(struct ucontext *trapframe)
{
    task_struct *parent = get_current_task();
    task_struct *child = task_create(0, 0);
    disable_interrupt();

    child->priority++;

    int kstack_offset = parent->kstack - (void *)trapframe;
    int ustack_offset = (unsigned long long)parent->ustack - trapframe->sp_el0;

    for (int i = 0; i < kstack_offset; i++) // copy kstack content
        *((char *)child->kstack - i) = *((char *)parent->kstack - i);

    for (int i = 0; i < ustack_offset; i++) // copy ustack content
        *((char *)child->ustack - i) = *((char *)parent->ustack - i);

    for (int i = 0; i < SIG_NUM; i++) // copy signal handler
    {
        child->is_default_signal_handler[i] = parent->is_default_signal_handler[i];
        child->signal_handler[i] = parent->signal_handler[i];
    }

    child->cpu_context = parent->cpu_context;
    child->cpu_context.sp = (unsigned long long)child->kstack - kstack_offset; // revise the right kernel stack pointer
    child->cpu_context.fp = (unsigned long long)child->kstack;
    child->cpu_context.lr = (unsigned long long)return_from_fork;

    struct ucontext *child_trapframe = (struct ucontext *)child->cpu_context.sp;
    child_trapframe->sp_el0 = (unsigned long long)child->ustack - ustack_offset; // revise the right user stack pointer

    trapframe->x[0] = child->id; // return child's id
    child_trapframe->x[0] = 0;
}

void sys_exit(struct ucontext *trapframe)
{
    task_exit();
}

void sys_mbox_call(struct ucontext *trapframe)
{
    unsigned char ch = (unsigned char)trapframe->x[0];
    unsigned int *mbox = (unsigned int *)trapframe->x[1];
    for (int i = 0; i < 36; i++)
        mailbox[i] = mbox[i];

    trapframe->x[0] = mailbox_call(ch);

    for (int i = 0; i < 36; i++)
        mbox[i] = mailbox[i];
}

void sys_kill(struct ucontext *trapframe)
{
    int pid = (int)trapframe->x[0];
    for (task_struct *cur = run_queue.head[0]; cur != NULL; cur = cur->next) // find the task
        if (cur->id == pid)
            cur->state = EXIT;
}

void sys_signal(struct ucontext *trapframe)
{
    int SIGNAL = (int)trapframe->x[0];
    void (*handler)() = (void (*)())trapframe->x[1];

    task_struct *cur = get_current_task();

    cur->is_default_signal_handler[SIGNAL] = 0;
    cur->signal_handler[SIGNAL] = handler;
}

void sys_signal_kill(struct ucontext *trapframe)
{
    int pid = (int)trapframe->x[0];
    int SIGNAL = (int)trapframe->x[1];
    for (task_struct *cur = run_queue.head[0]; cur != NULL; cur = cur->next) // find the task
        if (cur->id == pid)
            cur->received_signal = SIGNAL;
}

void sys_sigreturn(struct ucontext *trapframe)
{
    task_struct *cur = get_current_task();

    for (int i = 0; i < 34; i++)
        *((unsigned long long *)trapframe + i) = *((unsigned long long *)cur->signal_stack - i);

    kfree((char *)cur->signal_stack - 4096 * 2);
    cur->signal_stack = NULL;
}