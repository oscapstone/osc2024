#include "bcm2837/rpi_irq.h"
#include "bcm2837/rpi_uart1.h"
#include "uart1.h"
#include "exception.h"
#include "timer.h"
#include "heap.h"

// DAIF, Interrupt Mask Bits
void el1_interrupt_enable()
{
    __asm__ __volatile__("msr daifclr, 0xf"); // umask all DAIF
}

void el1_interrupt_disable()
{
    __asm__ __volatile__("msr daifset, 0xf"); // mask all DAIF
}


unsigned long long int lock_counter = 0;

void lock()
{
    el1_interrupt_disable();
    lock_counter++;
}

void unlock()
{
    lock_counter--;
    if (lock_counter < 0)
    {
        //uart_puts("lock counter error\r\n");
        while (1)
            ;
    }
    else if (lock_counter == 0)
    {
        el1_interrupt_enable();
    }
}

void el1h_irq_router()
{
    lock();
    // decouple the handler into irqtask queue
    // (1) https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf - Pg.113
    // (2) https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf - Pg.16
    if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) // from aux && from GPU0 -> uart exception
    {
        //if (*AUX_MU_IER_REG & 2)
        //else if (*AUX_MU_IER_REG & 1)
        if (*AUX_MU_IIR_REG & (1 << 1))
        {
            *AUX_MU_IER_REG &= ~(2); // disable write interrupt
            irqtask_add(uart_w_irq_handler, UART_IRQ_PRIORITY);
            unlock();
            irqtask_run_preemptive(); // run the queued task before returning to the program.
        }
        else if (*AUX_MU_IIR_REG & (2 << 1))
        {
            *AUX_MU_IER_REG &= ~(1); // disable read interrupt
            irqtask_add(uart_r_irq_handler, UART_IRQ_PRIORITY);
            unlock();
            irqtask_run_preemptive();
        }
    }
    else if (*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ) // from CNTPNS (core_timer) // A1 - setTimeout run in el1
    {
        core_timer_disable();
        irqtask_add(core_timer_handler, TIMER_IRQ_PRIORITY);
        unlock();
        irqtask_run_preemptive();
        core_timer_enable();
    }
}

void el0_sync_router()
{
    unsigned long long spsr_el1;
    __asm__ __volatile__("mrs %0, SPSR_EL1\n\t" : "=r"(spsr_el1)); // EL1 configuration, spsr_el1[9:6]=4b0 to enable interrupt
    unsigned long long elr_el1;
    __asm__ __volatile__("mrs %0, ELR_EL1\n\t" : "=r"(elr_el1)); // ELR_EL1 holds the address if return to EL1
    unsigned long long esr_el1;
    __asm__ __volatile__("mrs %0, ESR_EL1\n\t" : "=r"(esr_el1)); // ESR_EL1 holds symdrome information of exception, to know why the exception happens.
    
    uart_sendlinek("[Exception][el0_sync] spsr_el1 : 0x%x, elr_el1 : 0x%x, esr_el1 : 0x%x\n", spsr_el1, elr_el1, esr_el1);
}

void el0_irq_64_router()
{
}

void invalid_exception_router(unsigned long long x0)
{

    uart_sendlinek("\n invalid exception  %x \n", x0);
    while (1);


    // while(1);
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
    task_list = kmalloc(sizeof(list_head_t));
    INIT_LIST_HEAD(task_list);
}

void irqtask_add(void *task_function, unsigned long long priority)
{
    irqtask_t *the_task = kmalloc(sizeof(irqtask_t)); // free by irq_tasl_run_preemptive()

    // store all the related information into irqtask node
    // manually copy the device's buffer
    the_task->priority = priority;
    the_task->task_function = task_function;
    INIT_LIST_HEAD(&the_task->listhead);

    // add the timer_event into timer_event_list (sorted)
    // if the priorities are the same -> FIFO
    struct list_head *curr;

    // mask the device's interrupt line
    //el1_interrupt_disable();
    // enqueue the processing task to the event queue with sorting.
    list_for_each(curr, task_list)
    {
        if (((irqtask_t *)curr)->priority > the_task->priority)
        {
            list_add(&the_task->listhead, curr->prev);
            break;
        }
    }
    // if the priority is lowest
    if (list_is_head(curr, task_list))
    {
        list_add_tail(&the_task->listhead, task_list);
    }
    // unmask the interrupt line
    //el1_interrupt_enable();
}

void irqtask_run_preemptive()
{
    // el1_interrupt_enable();
    while (!list_empty(task_list))
    {
        // critical section protects new coming node

        lock();
        irqtask_t *the_task = (irqtask_t *)task_list->next;
        // Run new task (early return) if its priority is lower than the scheduled task.
        if (curr_task_priority <= the_task->priority)
        {
            unlock();
            break;
        }
        // get the scheduled task and run it.
        list_del_entry((struct list_head *)the_task);
        int prev_task_priority = curr_task_priority;
        curr_task_priority = the_task->priority;
        // uart_sendlinek("preemptive curr_task_priority: %d\r\n", curr_task_priority);
        unlock();

        irqtask_run(the_task);

        lock();
        curr_task_priority = prev_task_priority;
        unlock();
        // free(the_task);
    }
}

void irqtask_run(irqtask_t *the_task)
{
    ((void (*)())the_task->task_function)();
}
