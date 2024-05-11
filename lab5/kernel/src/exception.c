#include "gpio.h"
#include "uart.h"
#include "task.h"
#include "timer.h"
#include "schedule.h"
#include "exception.h"

#define AUX_MU_IO ((volatile unsigned int *)(MMIO_BASE + 0x00215040))
#define AUX_MU_IER ((volatile unsigned int *)(MMIO_BASE + 0x00215044))
#define AUX_MU_IIR ((volatile unsigned int *)(MMIO_BASE + 0x00215048))
#define AUX_MU_LSR ((volatile unsigned int *)(MMIO_BASE + 0x00215054))

#define CORE0_INTERRUPT_SOURCE (volatile unsigned int *)0x40000060

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

void el0_svc_handler_entry(struct trapframe *trapframe)
{
    disable_interrupt();

    int get_syscall_no = trapframe->x[8];
    if (get_syscall_no == SYS_GET_PID)
        sys_getpid(trapframe);
    else if (get_syscall_no == SYS_UART_READ)
        sys_uartread(trapframe);
    else if (get_syscall_no == SYS_UART_WRITE)
        sys_uartwrite(trapframe);
    else if (get_syscall_no == SYS_EXEC)
        sys_exec(trapframe);
    else if (get_syscall_no == SYS_FORK)
        sys_fork(trapframe);
    else if (get_syscall_no == SYS_EXIT)
        sys_exit(trapframe);
        
    enable_interrupt();
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

void sys_getpid(struct trapframe *trapframe)
{
    trapframe->x[0] = get_current_task()->id;
}

void sys_uartread(struct trapframe *trapframe)
{
    char *buf = (char *)trapframe->x[0];
    int size = trapframe->x[1];

    for (int i = 0; i < size; i++)
        buf[i] = uart_read();

    buf[size] = '\0';
    trapframe->x[0] = size;
}

void sys_uartwrite(struct trapframe *trapframe)
{
    char *buf = (char *)trapframe->x[0];
    int size = trapframe->x[1];

    for (int i = 0; i < size; i++)
        uart_write(buf[i]);

    trapframe->x[0] = size;
}

void sys_exec(struct trapframe *trapframe)
{
    void (*func)() = (void (*)())trapframe->x[0];
    do_exec(func);
    trapframe->x[0] = -1; // if do_exec is falut
}

void sys_fork(struct trapframe *trapframe)
{
    task_struct *parent = get_current_task();
    task_struct *child = task_create(0, 0);
    child->priority++;

    int kstack_offset = parent->kstack - (void *)trapframe;
    int ustack_offset = (unsigned long long)parent->ustack - trapframe->sp_el0;

    for (int i = 0; i < kstack_offset; i++) // copy kstack content
        *((char *)child->kstack - i) = *((char *)parent->kstack - i);

    for (int i = 0; i < ustack_offset; i++) // copy ustack content
        *((char *)child->ustack - i) = *((char *)parent->ustack - i);

    child->cpu_context = parent->cpu_context;
    child->cpu_context.sp = (unsigned long long)child->kstack - kstack_offset; // revise the right kernel stack pointer
    child->cpu_context.fp = (unsigned long long)child->kstack;
    child->cpu_context.lr = (unsigned long long)return_from_fork;

    struct trapframe *child_trapframe = (struct trapframe *)child->cpu_context.sp; // revise the right user pointer
    child_trapframe->sp_el0 = (unsigned long long)child->ustack - ustack_offset;

    trapframe->x[0] = child->id; // return child's id
    child_trapframe->x[0] = 0;
}

void sys_exit(struct trapframe *trapframe)
{
    task_exit();
}