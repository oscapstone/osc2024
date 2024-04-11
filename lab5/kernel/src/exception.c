#include "bcm2837/rpi_irq.h"
#include "bcm2837/rpi_uart1.h"
#include "uart1.h"
#include "exception.h"
#include "timer.h"
#include "memory.h"
#include "syscall.h"
#include "sched.h"
#include "signal.h"

extern list_head_t *run_queue;

// DAIF, Interrupt Mask Bits
void el1_interrupt_enable(){
    __asm__ __volatile__("msr daifclr, 0xf"); // umask all DAIF
}

void el1_interrupt_disable(){
    __asm__ __volatile__("msr daifset, 0xf"); // mask all DAIF
}

static unsigned long long lock_count = 0;
void lock()
{
    el1_interrupt_disable();
    lock_count++;
}

void unlock()
{
    lock_count--;
    if (lock_count == 0)
        el1_interrupt_enable();
}

void el1h_irq_router(trapframe_t *tpf){
    // decouple the handler into irqtask queue
    // (1) https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf - Pg.113
    // (2) https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf - Pg.16
    if(*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) // from aux && from GPU0 -> uart exception
    {
        if (*AUX_MU_IER_REG & (1 << 1))
        {
            *AUX_MU_IIR_REG &= ~(2);  // disable write interrupt
            irqtask_add(uart_w_irq_handler, UART_IRQ_PRIORITY);
            irqtask_run_preemptive(); // run the queued task before returning to the program.
        }
        else if (*AUX_MU_IER_REG & (2 << 1))
        {
            *AUX_MU_IIR_REG &= ~(1);  // disable read interrupt
            irqtask_add(uart_r_irq_handler, UART_IRQ_PRIORITY);
            irqtask_run_preemptive();
        }
    }
    else if(*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ)  //from CNTPNS (core_timer) // A1 - setTimeout run in el1
    {
        core_timer_disable();
        irqtask_add(core_timer_handler, TIMER_IRQ_PRIORITY);
        irqtask_run_preemptive();
        core_timer_enable();

        //at least two threads running -> schedule for any timer irq
        if (run_queue->next->next != run_queue) schedule();
    }

    //only do signal handler when return to user mode
    if ((tpf->spsr_el1 & 0b1100) == 0)
    {
        check_signal(tpf);
    }
}

void el0_sync_router(trapframe_t *tpf){

    // Basic #3 - Based on System Call Format in Video Player’s Test Program

    el1_interrupt_enable(); // Allow UART input during exception
    unsigned long long syscall_no = tpf->x8;

    if (syscall_no == 0)
    {
        getpid(tpf);
    }
    else if(syscall_no == 1)
    {
        uartread(tpf, (char *)tpf->x0, tpf->x1);
    }
    else if (syscall_no == 2)
    {
        uartwrite(tpf, (char *)tpf->x0, tpf->x1);
    }
    else if (syscall_no == 3)
    {
        exec(tpf, (char *)tpf->x0, (char **)tpf->x1);
    }
    else if (syscall_no == 4)
    {
        fork(tpf);
    }
    else if (syscall_no == 5)
    {
        exit(tpf, tpf->x0);
    }
    else if (syscall_no == 6)
    {
        syscall_mbox_call(tpf, (unsigned char)tpf->x0, (unsigned int *)tpf->x1);
    }
    else if (syscall_no == 7)
    {
        kill(tpf, (int)tpf->x0);
    }
    else if (syscall_no == 8)
    {
        signal_register(tpf->x0, (void (*)())tpf->x1);
    }
    else if (syscall_no == 9)
    {
        signal_kill(tpf->x0, tpf->x1);
    }
    else if (syscall_no == 50)
    {
        sigreturn(tpf);
    }
}

void el0_irq_64_router(trapframe_t *tpf){
    // decouple the handler into irqtask queue
    // (1) https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf - Pg.113
    // (2) https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf - Pg.16
    if(*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) // from aux && from GPU0 -> uart exception
    {
        if (*AUX_MU_IER_REG & (0b01 << 1))
        {
            *AUX_MU_IIR_REG &= ~(2);  // disable write interrupt
            irqtask_add(uart_w_irq_handler, UART_IRQ_PRIORITY);
            irqtask_run_preemptive();
        }
        else if (*AUX_MU_IER_REG & (0b10 << 1))
        {
            *AUX_MU_IIR_REG &= ~(1);  // disable read interrupt
            irqtask_add(uart_r_irq_handler, UART_IRQ_PRIORITY);
            irqtask_run_preemptive();
        }
    }
    else if(*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ)  //from CNTPNS (core_timer) // A1 - setTimeout run in el1
    {
        core_timer_disable();
        irqtask_add(core_timer_handler, TIMER_IRQ_PRIORITY);
        irqtask_run_preemptive();
        core_timer_enable();

        //at least two trhead running -> schedule for any timer irq
        if (run_queue->next->next != run_queue) schedule();
    }

    //only do signal handler when return to user mode
    if ((tpf->spsr_el1 & 0b1100) == 0)
    {
        check_signal(tpf);
    }
}


void invalid_exception_router(unsigned long long x0){
    //uart_sendline("invalid exception : 0x%x\r\n",x0);
    //while(1);
}

// ------------------------------------------------------------------------------------------

/*
Preemption
Now, any interrupt handler can preempt the task’s execution, but the newly enqueued task still needs to wait for the currently running task’s completion.
It’d be better if the newly enqueued task with a higher priority can preempt the currently running task.
To achieve the preemption, the kernel can check the last executing task’s priority before returning to the previous interrupt handler.
If there are higher priority tasks, execute the highest priority task.
*/

int curr_task_priority = 9999;   // Small number has higher priority

struct list_head *task_list;
void irqtask_list_init()
{
    INIT_LIST_HEAD(task_list);
}


void irqtask_add(void *task_function,unsigned long long priority){
    irqtask_t *the_task = s_allocator(sizeof(irqtask_t)); // free by irq_tasl_run_preemptive()

    // store all the related information into irqtask node
    // manually copy the device's buffer
    the_task->priority = priority;
    the_task->task_function = task_function;
    INIT_LIST_HEAD(&the_task->listhead);

    // add the timer_event into timer_event_list (sorted)
    // if the priorities are the same -> FIFO
    struct list_head *curr;

    // mask the device's interrupt line
    lock();
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
    unlock();
}

void irqtask_run_preemptive(){
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

        unlock();
        irqtask_run(the_task);
        lock();

        curr_task_priority = prev_task_priority;
        unlock();
        s_free(the_task);
    }
}

void irqtask_run(irqtask_t* the_task)
{
    ((void (*)())the_task->task_function)();
}


