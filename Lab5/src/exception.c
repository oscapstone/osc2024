#include "bcm2837/rpi_irq.h"
#include "bcm2837/rpi_uart1.h"
#include "uart.h"
#include "exception.h"
#include "timer.h"
#include "memory.h"
#include "syscall.h"
#include "schedule.h"
#include "signal.h"

extern list_head_t *run_queue;

// DAIF, Interrupt Mask Bits
void enable_irq()
{
    __asm__ __volatile__("msr daifclr, 0xf"); // umask all DAIF
}

void disable_irq()
{
    __asm__ __volatile__("msr daifset, 0xf"); // mask all DAIF
}


void irq_router(trapframe_t *tp)
{
    // bcm2835 - Pg.113
    // bcm2836 - Pg.16
    if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) // from aux && from GPU0 -> uart exception
    {
        if (*AUX_MU_IIR_REG & (0b01 << 1))
        {
            *AUX_MU_IER_REG &= ~(2); // disable write interrupt
            add_task_list(uart_w_irq_handler, UART_PRIORITY);
            preemption(); // run the queued task before returning to the program.
        }
        else if (*AUX_MU_IIR_REG & (0b10 << 1))
        {
            *AUX_MU_IER_REG &= ~(1); // disable read interrupt
            add_task_list(uart_r_irq_handler, UART_PRIORITY);
            preemption();
        }
    }
    else if (*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ) // from CNTPNS (core_timer) // A1 - setTimeout run in el1
    {
        core_timer_disable();
        add_task_list(core_timer_handler, TIMER_PRIORITY);
        preemption();
        core_timer_enable();

        if (run_queue->next->next != run_queue)
            schedule();
    }

    // spsr[3:2]: EL  spsr[1]: set 0 spsr[0]: using what EL's sp
    // exception from EL0
    if ((tp->spsr_el1 & 0b1100) == 0)
    {
        check_signal(tp);
    }
}

void svc_router(trapframe_t *tp)
{
    enable_irq();
    unsigned long long syscall = tp->x8;
    
    if (syscall == 0)
    {
        getpid(tp);
    }
    else if (syscall == 1)
    {
        uartread(tp, (char *)tp->x0, tp->x1);
    }
    else if (syscall == 2)
    {
        uartwrite(tp, (char *)tp->x0, tp->x1);
    }
    else if (syscall == 3)
    {
        exec(tp, (char *)tp->x0, (char **)tp->x1);
    }
    else if (syscall == 4)
    {
        fork(tp);
    }
    else if (syscall == 5)
    {
        exit(tp, tp->x0);
    }
    else if (syscall == 6)
    {
        syscall_mbox_call(tp, (unsigned char)tp->x0, (unsigned int *)tp->x1);
    }
    else if (syscall == 7)
    {
        kill(tp, (int)tp->x0);
    }
    else if (syscall == 8)
    {
        register_signal(tp->x0, (void (*)())tp->x1);
    }
    else if (syscall == 9)
    {
        signal_kill(tp->x0, tp->x1);
    }
    else if (syscall == 10)
    {
        sigreturn(tp);
    }
}

void invalid_exception_router(unsigned long long x0)
{
    uart_sendline("%x: invalid exception\n", x0);
    unsigned long long tmp;
    __asm__ __volatile__("mrs %0, esr_el1": "=r"(tmp));
    uart_sendline("exception syndrome: %x", tmp);
    while (1)
    {
    }
}

int cur_priority = 9;

struct list_head *task_list;

void task_list_init()
{
    INIT_LIST_HEAD(task_list);
}

// add new task to task list
void add_task_list(void *callback, unsigned long long priority)
{
    irqtask_t *cur_task = s_allocator(sizeof(irqtask_t));

    cur_task->priority = priority;
    cur_task->callback = callback;
    INIT_LIST_HEAD(&cur_task->listhead);

    struct list_head *ptr;

    disable_irq();

    list_for_each(ptr, task_list)
    {
        if (((irqtask_t *)ptr)->priority > cur_task->priority)
        {
            list_add(&cur_task->listhead, ptr->prev);
            break;
        }
    }

    if (list_is_head(ptr, task_list))
    {
        list_add_tail(&cur_task->listhead, task_list);
    }

    enable_irq();
}

// decide preemtion or not
void preemption()
{
    //lock();
    while (!list_empty(task_list))
    {
        disable_irq();
        irqtask_t *first_task = (irqtask_t *)task_list->next;

        // process executing now has highest priority
        if (cur_priority <= first_task->priority)
        {
            enable_irq();
            break;
        }

        // executing first task in the task bc its priority is higher
        list_del_entry((struct list_head *)first_task);
        int prev_priority = cur_priority;
        cur_priority = first_task->priority;

        enable_irq();
        run_task(first_task);
        disable_irq();

        cur_priority = prev_priority;
        enable_irq();
        s_free(first_task);
    }
}

// run task callback
void run_task(irqtask_t *task)
{
    ((void (*)())task->callback)();
}
