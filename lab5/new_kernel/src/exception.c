#include "exception_table.h"
#include "timer.h"
#include "rpi_irq.h"
#include "rpi_mini_uart.h"
#include "rpi_gpio.h"
#include "mini_uart.h"
#include "utility.h"
#include "exception.h"
#include "memory.h"

struct list_head *task_list;
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
        uart_puts("lock counter error\r\n");
        while (1)
            ;
    }
    else if (lock_counter == 0)
    {
        el1_interrupt_enable();
    }
}
void irqtask_list_init()
{   
    task_list = kmalloc(sizeof(irqtask_t));

    INIT_LIST_HEAD(task_list);
}

void irqtask_add(void *task_function, unsigned long long priority)
{
    irqtask_t *the_task = kmalloc(sizeof(irqtask_t)); 

    // store all the related information into irqtask node
    // manually copy the device's buffer
    the_task->priority = priority;
    the_task->task_function = task_function;
    INIT_LIST_HEAD(&(the_task->listhead));

    // add the timer_event into timer_event_list (sorted)
    // if the priorities are the same -> FIFO
    struct list_head *curr;

    // mask the device's interrupt line
    el1_interrupt_disable();
    // enqueue the processing task to the event queue with sorting.
    list_for_each(curr, task_list)
    {
        if (((irqtask_t *)curr)->priority > the_task->priority)
        {
            list_add_tail(&(the_task->listhead), task_list);
            break;
        }
    }
    // if the priority is lowest
    if (list_is_head(curr, task_list))
    {
        list_add_tail(&the_task->listhead, task_list);
    }
    // unmask the interrupt line
    // el1_interrupt_enable();
}
int curr_task_priority = 9999; // Small number has higher priority

void irqtask_run_preemptive()
{   
    //倘若還有irq要處理
    while (!list_empty(task_list))
    {
        lock();
        //給一塊 the_task空間，存放接下來要執行的task
        irqtask_t *the_task = (irqtask_t *)task_list->next;
        //一塊current list head
        struct list_head *curr;

        //倘若現階段的task 優先高於 下一個要執行的，break離開，不執行下一段
        if (curr_task_priority <= the_task->priority)
        {   
            unlock();
            break;
        }
        //到此為有task 要搶佔了，將the_task 移除佇列，因為要執行了
        list_del_entry((struct list_head *)the_task);

        //將被插隊的task 優先留下
        int prev_task_priority = curr_task_priority;
        // 把目前要執行的task priority設為現在要執行的task
        curr_task_priority = the_task->priority;
        
        unlock();
        
        //the task搶佔
        irqtask_run(the_task);
        
        lock();

        //搶佔的執行完畢 即可還原原本的執行模式
        curr_task_priority = prev_task_priority;
        kfree(the_task); // Adding at 6/13 for lab5 
        unlock();
    }
}

void irqtask_run(irqtask_t *the_task)
{
    ((void (*)())the_task->task_function)();
}

void sync_exc_router(unsigned long spsr, unsigned long elr, unsigned long esr)
{
    int ec = (esr >> 26) & 0b111111;
    if (ec == 0b010101)
    { // system call
        uart_puts("[Syscall] \n");
        uart_puts("Saved Program Status (SPSR): 0x");
        uart_hex(spsr);
        uart_puts("\n");

        uart_puts("Exception return address (ELR): 0x");
        uart_hex(elr);
        uart_puts("\n");

        uart_puts("Exception syndrome (ESR): 0x");
        uart_hex(esr);
        uart_puts("\n");
    }
    else
    {
        uart_puts("Not svc syscall but el0 syn router \n");
        while (1)
        {
            /* code */
        }
        
    }
}
void breakpt(){

}

void breakpt2(){

}
void irq_exc_router()
{   
    lock();

    // GPU IRQ 57: UART Interrupt
    // if (irq_basic_pending & (1 << 19)) {
    //     uart_intr_handler();
    // }
    // decouple the handler into irqtask queue
    // (1) https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf - Pg.113
    // (2) https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf - Pg.16
    if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTR_SRC & INTERRUPT_SOURCE_GPU) // from aux && from GPU0 -> uart exception
    {   
        breakpt();
        if (*AUX_MU_IER_REG & 2)
        {   // & (0b10) : enable write

            *AUX_MU_IER_REG &= ~(2); // disable write interrupt
            irqtask_add(uart_w_irq_handler, UART_IRQ_PRIORITY);
            unlock();
            irqtask_run_preemptive(); // run the queued task before returning to the program.
        }
        else if (*AUX_MU_IER_REG & 1)
        {
            // & (0b01) : enable read 
            *AUX_MU_IER_REG &= ~(1); // disable read interrupt
            irqtask_add(uart_r_irq_handler, UART_IRQ_PRIORITY);
            unlock();
            irqtask_run_preemptive();
        }
    }
    else if (*CORE0_INTR_SRC & INTERRUPT_SOURCE_CNTPNSIRQ) // from CNTPNS (core_timer) // A1 - setTimeout run in el1
    {   
        // uart_puts("timer interr\n");
        core_timer_disable();
        irqtask_add(core_timer_hadler, TIMER_IRQ_PRIORITY);
        unlock();
        irqtask_run_preemptive();
        core_timer_enable();
    }
    else
    {
        uart_puts("Hello World el1 64 router other interrupt!\r\n");
    }


}

void not_implemented()
{
    uart_puts("kenel panic because of not implemented function...\n");
    while (1);
}

void show_exception_status(int type, unsigned long esr, unsigned long address)
{
    uart_puts("not now for Exception status.. \n");

}
