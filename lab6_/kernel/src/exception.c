#include "bcm2837/rpi_irq.h"
#include "bcm2837/rpi_uart1.h"
#include "uart1.h"
#include "exception.h"
#include "timer.h"
#include "memory.h"
#include "syscall.h"
#include "sched.h"
#include "signal.h"

extern int finish_init_thread_sched;
extern int syscall_num;
extern SYSCALL_TABLE_T *syscall_table;

// DAIF, Interrupt Mask Bits
void el1_interrupt_enable(){
    __asm__ __volatile__("msr daifclr, 0xf"); // umask all DAIF
}

void el1_interrupt_disable(){
    __asm__ __volatile__("msr daifset, 0xf"); // mask all DAIF
}

int get_current_el()
{
    int el;
    __asm__ __volatile__("mrs %0, CurrentEL" : "=r"(el));
    return el >> 2;
}
static unsigned long long lock_counter = 0;

void lock()
{
    // uart_sendline("lock %d\r\n", lock_counter);
    el1_interrupt_disable();
    lock_counter++;
}

void unlock()
{
    // uart_sendline("unlock %d\r\n", lock_counter);
    lock_counter--;
    if (lock_counter < 0)
    {
        uart_puts("lock counter error\r\n");
        while (1)
            ;
    }
    else if (lock_counter == 0)
    {
        el1_interrupt_enable();
    }
}

void el1h_irq_router(trapframe_t *tpf){
    lock();
    // uart_sendline("irq_router\r\n");
    // decouple the handler into irqtask queue
    // (1) https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf - Pg.113
    // (2) https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf - Pg.16
    if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) // from aux && from GPU0 -> uart exception
    {
        if (*AUX_MU_IER_REG & 2)
        {
            // uart_sendline("write interrupt\r\n");
            *AUX_MU_IER_REG &= ~(2); // disable write interrupt
            irqtask_add(uart_w_irq_handler, UART_IRQ_PRIORITY);
            unlock();
            irqtask_run_preemptive(); // run the queued task before returning to the program.
        }
        else if (*AUX_MU_IER_REG & 1)
        {
            // uart_sendline("read interrupt\r\n");
            *AUX_MU_IER_REG &= ~(1); // disable read interrupt
            irqtask_add(uart_r_irq_handler, UART_IRQ_PRIORITY);
            unlock();
            irqtask_run_preemptive();
        }
    }
    else if(*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ)  //from CNTPNS (core_timer) // A1 - setTimeout run in el1
    {
        core_timer_disable();
        irqtask_add(core_timer_handler, TIMER_IRQ_PRIORITY);
        unlock();
        irqtask_run_preemptive();
        core_timer_enable();

        // check whether init_thread_sched is finished
        if (finish_init_thread_sched == 1)
            schedule();
    }
    else
    {
        uart_puts("Hello World el1 64 router other interrupt!\r\n");
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

    if (syscall_no >= syscall_num || syscall_no < 0)
    {
        // invalid syscall number
        uart_sendline("Invalid syscall number: %d\r\n", syscall_no);
        tpf->x0 = -1;
        return;
    }
    if (syscall_no == 50) syscall_no = 10;
    // char* func = (char*)(((&syscall_table)[syscall_no]));
    // ((int (*)(trapframe_t *))func)(tpf);
    ((int (*)(trapframe_t *))(((&syscall_table)[syscall_no])))(tpf);
}

void el0_irq_64_router(trapframe_t *tpf){
    lock();
    // decouple the handler into irqtask queue
    // (1) https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf - Pg.113
    // (2) https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf - Pg.16
    if(*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) // from aux && from GPU0 -> uart exception
    {
       if (*AUX_MU_IER_REG & 2)
        {
            *AUX_MU_IER_REG &= ~(2); // disable write interrupt
            irqtask_add(uart_w_irq_handler, UART_IRQ_PRIORITY);
            unlock();
            irqtask_run_preemptive(); // run the queued task before returning to the program.
        }
        else if (*AUX_MU_IER_REG & 1)
        {
            *AUX_MU_IER_REG &= ~(1); // disable read interrupt
            irqtask_add(uart_r_irq_handler, UART_IRQ_PRIORITY);
            unlock();
            irqtask_run_preemptive();
        }
    }
    else if(*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ)  //from CNTPNS (core_timer) // A1 - setTimeout run in el1
    {
        core_timer_disable();
        irqtask_add(core_timer_handler, TIMER_IRQ_PRIORITY);
        unlock();
        irqtask_run_preemptive();
        core_timer_enable();

        // check whether init_thread_sched is finished
        if (finish_init_thread_sched == 1)
            schedule();    
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
    task_list = kmalloc(sizeof(struct list_head));
    INIT_LIST_HEAD(task_list);
}


void irqtask_add(void *task_function, unsigned long long priority)
{
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
        lock();
        irqtask_t *the_task = (irqtask_t *)task_list->next;
        if (curr_task_priority <= the_task->priority)
        {
            unlock();
            break;
        }
        list_del_entry((struct list_head *)the_task);
        int prev_task_priority = curr_task_priority;
        curr_task_priority = the_task->priority;
        
        unlock();
        
        irqtask_run(the_task);
        
        lock();

        curr_task_priority = prev_task_priority;
        init_free(the_task);
        unlock();
    }
}

void irqtask_run(irqtask_t *the_task)
{
    ((void (*)())the_task->task_function)();
}