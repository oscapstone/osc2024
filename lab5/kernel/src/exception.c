#include "bcm2837/rpi_irq.h"
#include "bcm2837/rpi_uart1.h"
#include "uart1.h"
#include "exception.h"
#include "timer.h"
#include "memory.h"
#include "stdio.h"
#include "stdint.h"
#include "syscall.h"

extern int8_t need_to_schedule;

// DAIF, Interrupt Mask Bits
void el1_interrupt_enable()
{
    __asm__ __volatile__("msr daifclr, 0xf"); // umask all DAIF
}

void el1_interrupt_disable()
{
    __asm__ __volatile__("msr daifset, 0xf"); // mask all DAIF
}

static uint64_t lock_counter = 0;

void lock_interrupt()
{
    el1_interrupt_disable();
    lock_counter++;
}

void unlock_interrupt()
{
    lock_counter--;
    if (lock_counter < 0)
    {
        ERROR("lock_interrupt counter error");
        while (1)
            ;
    }
    else if (lock_counter == 0)
    {
        el1_interrupt_enable();
    }
}

void el1h_irq_router(trapframe_t *tpf)
{
    lock_interrupt();
    // decouple the handler into irqtask queue
    // (1) https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf - Pg.113
    // (2) https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf - Pg.16
    if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) // from aux && from GPU0 -> uart exception
    {
        if (*AUX_MU_IER_REG & 2)
        {
            *AUX_MU_IER_REG &= ~(2); // disable write interrupt
            irqtask_add(uart_w_irq_handler, UART_IRQ_PRIORITY);
            unlock_interrupt();
            irqtask_run_preemptive(); // run the queued task before returning to the program.
        }
        else if (*AUX_MU_IER_REG & 1)
        {
            *AUX_MU_IER_REG &= ~(1); // Re-enable core timer interrupt when entering core_timer_handler or add_timer
            irqtask_add(uart_r_irq_handler, UART_IRQ_PRIORITY);
            unlock_interrupt();
            irqtask_run_preemptive();
        }
    }
    else if (*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ) // from CNTPNS (core_timer) // A1 - setTimeout run in el1
    {
        core_timer_disable(); // enable core timer interrupt when entering the handler
        irqtask_add(core_timer_handler, TIMER_IRQ_PRIORITY);
        unlock_interrupt();
        irqtask_run_preemptive();
    }
    else
    {
        uart_puts("Hello World el1 64 router other interrupt!\r\n");
    }
    if (need_to_schedule == 1)
    {
        need_to_schedule = 0;
        schedule();
    }
}

void el0_sync_router(trapframe_t *tpf)
{
    // Basic #3 - Based on System Call Format in Video Player’s Test Program
    uint64_t syscall_no = tpf->x8 >= MAX_SYSCALL ? MAX_SYSCALL : tpf->x8;

    // only work with GCC
    void *syscall_router[] = {&&__getpid_label, &&__uart_read_label, &&__uart_write_label, &&__exec_label,
                              &&__fork_label, &&__exit_label, &&__mbox_call_label, &&__invalid_syscall_label};

    goto *syscall_router[syscall_no];

__getpid_label:
    tpf->x0 = syscall_getpid(tpf, (char *)tpf->x0, tpf->x1);
    return;

__uart_read_label:
    tpf->x0 = syscall_uart_read(tpf, (char *)tpf->x0, tpf->x1);
    return;

__uart_write_label:
    tpf->x0 = syscall_uart_write(tpf, (char *)tpf->x0, (char **)tpf->x1);
    return;

__exec_label:
    tpf->x0 = syscall_exec(tpf, (char *)tpf->x0, (char **)tpf->x1);
    return;

__fork_label:
    tpf->x0 = syscall_fork(tpf);
    return;

__exit_label:
    tpf->x0 = syscall_exit(tpf, tpf->x0);
    return;

__mbox_call_label:
    tpf->x0 = syscall_mbox_call(tpf, (uint8_t)tpf->x0, (unsigned int *)tpf->x1);
    return;

__invalid_syscall_label:
    ERROR("Invalid system call number: %d\r\n", syscall_no);
    return;
    
}

void el0_irq_64_router(trapframe_t *tpf)
{
    // decouple the handler into irqtask queue
    // (1) https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf - Pg.113
    // (2) https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf - Pg.16
    // if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) // from aux && from GPU0 -> uart exception
    // {
    //     if (*AUX_MU_IIR_REG & (0b01 << 1))
    //     {
    //         *AUX_MU_IER_REG &= ~(2); // disable write interrupt
    //         irqtask_add(uart_w_irq_handler, UART_IRQ_PRIORITY);
    //         irqtask_run_preemptive();
    //     }
    //     else if (*AUX_MU_IIR_REG & (0b10 << 1))
    //     {
    //         *AUX_MU_IER_REG &= ~(1); // disable read interrupt
    //         irqtask_add(uart_r_irq_handler, UART_IRQ_PRIORITY);
    //         irqtask_run_preemptive();
    //     }
    // }
    // else if (*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ) // from CNTPNS (core_timer) // A1 - setTimeout run in el1
    // {
    //     core_timer_disable();
    //     irqtask_add(core_timer_handler, TIMER_IRQ_PRIORITY);
    //     irqtask_run_preemptive();
    // }
    uart_puts("Hello world! el0_irq_64_router!\r\n");
}

void invalid_exception_router(uint64_t x0)
{
    ERROR("invalid exception router: %d\r\n", x0);
    while (1)
        ;
}

// ------------------------------------------------------------------------------------------

/*
Preemption
Now, any interrupt handler can preempt the task’s execution, but the newly enqueued task still needs to wait for the currently running task’s completion.
It’d be better if the newly enqueued task with a higher priority can preempt the currently running task.
To achieve the preemption, the kernel can check the last executing task’s priority before returning to the previous interrupt handler.
If there are higher priority tasks, execute the highest priority task.
*/

int curr_task_priority = 9999; // Small number has higher priority

struct list_head *task_list;
void irqtask_list_init()
{
    task_list = kmalloc(sizeof(irqtask_t));
    INIT_LIST_HEAD(task_list);
}

void irqtask_add(void *task_function, uint64_t priority)
{
    if (task_function == uart_r_irq_handler)
        DEBUG("irqtask_add uart_r_irq_handler, kmalloc\r\n");
    else if (task_function == uart_w_irq_handler)
        DEBUG("irqtask_add uart_w_irq_handler, kmalloc\r\n");
    else if (task_function == core_timer_handler)
        DEBUG("irqtask_add core_timer_handler, kmalloc\r\n");
    irqtask_t *the_task = kmalloc(sizeof(irqtask_t)); // free by irq_tasl_run_preemptive()
    // store all the related information into irqtask node
    // manually copy the device's buffer
    the_task->priority = priority;
    the_task->task_function = task_function;
    INIT_LIST_HEAD(&(the_task->listhead));

    // add the timer_event into timer_event_list (sorted)
    // if the priorities are the same -> FIFO
    struct list_head *curr;

    // enqueue the processing task to the event queue with sorting.
    list_for_each(curr, task_list)
    {
        if (((irqtask_t *)curr)->priority > the_task->priority)
        {
            list_add(&(the_task->listhead), curr->prev);
            break;
        }
    }
    // if the priority is lowest
    if (list_is_head(curr, task_list))
    {
        list_add_tail(&(the_task->listhead), task_list);
    }
}

void irqtask_run_preemptive()
{
    while (!list_empty(task_list))
    {
        lock_interrupt();
        irqtask_t *the_task = (irqtask_t *)task_list->next;
        // struct list_head *curr;
        // uart_puts("---------------------- list for each ----------------------\r\n");
        // list_for_each(curr, task_list)
        // {
        //     if (((irqtask_t *)curr)->task_function == uart_r_irq_handler)
        //         uart_puts("irqtask_run_preemptive uart_r_irq_handler\r\n");
        //     else if (((irqtask_t *)curr)->task_function == uart_w_irq_handler)
        //         uart_puts("irqtask_run_preemptive uart_w_irq_handler\r\n");
        //     else if (((irqtask_t *)curr)->task_function == core_timer_handler)
        //         uart_puts("irqtask_run_preemptive core_timer_handler\r\n");
        // }
        // uart_puts("--------------------------- end ---------------------------\r\n");
        // Run new task (early return) if its priority is lower than the scheduled task.
        if (curr_task_priority <= the_task->priority)
        {
            DEBUG("irqtask_run_preemptive early return\r\n");
            unlock_interrupt();
            break;
        }
        list_del_entry((struct list_head *)the_task);
        int prev_task_priority = curr_task_priority;
        curr_task_priority = the_task->priority;

        DEBUG("irqtask_run\r\n");
        unlock_interrupt();
        irqtask_run(the_task);

        lock_interrupt();

        curr_task_priority = prev_task_priority;
        DEBUG("irqtask_run_preemptive kfree\r\n");
        kfree(the_task);
        unlock_interrupt();
    }
}

void irqtask_run(irqtask_t *the_task)
{
    ((void (*)())the_task->task_function)();
}
