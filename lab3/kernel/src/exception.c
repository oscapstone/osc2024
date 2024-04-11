#include "bcm2837/rpi_irq.h"
#include "bcm2837/rpi_uart1.h"
#include "uart1.h"
#include "exception.h"
#include "timer.h"
#include "heap.h"

// DAIF, Interrupt Mask Bits
void el1_interrupt_enable(){
    __asm__ __volatile__("msr daifclr, 0xf"); // umask all DAIF
}

void el1_interrupt_disable(){
    __asm__ __volatile__("msr daifset, 0xf"); // mask all DAIF
}

void el1h_irq_router(){
    // decouple the handler into irqtask queue
    // (1) https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf - Pg.113
    // (2) https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf - Pg.16
    if(*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) // from aux && from GPU0 -> uart exception
    // IRQ_PENDING_1 暫存器的那些bit為1，就代表對應的中段源產生中斷請求
    // IRQ_PENDING_1的第29位代表uart是否發生IRQ ; 1:uart有中斷請求  0:無
    // define IRQ_PENDING_1_AUX_INT (1<<29)  
    // IRQ_PENDING_1(若29bit為1) & IRQ_PENDING_1_AUX_INT =1 代表uart有發出中斷請求
    // CORE0_INTERRUPT_SOURCE 暫存器的那些bit為1，就代表不同的事件處發中斷 
    // define INTERRUPT_SOURCE_GPU (1<<8) 
    // *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU:代表有與GPU相關的中斷
    // 檢查(1)uart (2)GPU都要發生中斷 ; 由GPU發送到core0
    {
        //uart中斷
        if (*AUX_MU_IIR_REG & (1 << 1))
        // AUX_MU_IIR_REG 2's bit=1 有write導致的interrupt
        // buffer可以寫入了，發出中斷告訴kernel
        {
            *AUX_MU_IER_REG &= ~(2);  // disable write interrupt
            // 處理這個寫中斷
            irqtask_add(uart_w_irq_handler, UART_IRQ_PRIORITY);
            // 把這個寫中斷加入task queue
            irqtask_run_preemptive(); // run the queued task before returning to the program.
            // 立即執行queue中的task
        }
        else if (*AUX_MU_IIR_REG & (2 << 1))
        // AUX_MU_IIR_REG 3's bit=1 有read導致的interrupt
        // buffer收到資料了，發出中斷告訴kernel
        {
            *AUX_MU_IER_REG &= ~(1);  // disable read interrupt
            irqtask_add(uart_r_irq_handler, UART_IRQ_PRIORITY);
            irqtask_run_preemptive();
        }
    }
    // core0中斷
    else if(*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ)  //from CNTPNS (core_timer) // A1 - setTimeout run in el1
    // 檢查是否有core0 timer interrupt
    // define INTERRUPT_SOURCE_CNTPNSIRQ (1<<1)
    // CNTPNSIRQ: [spec] counter physical non-secure IRQ; 非安全世界中的物理計時器中斷 ???
    {
        core_timer_disable();
        // 進用core timer
        irqtask_add(core_timer_handler, TIMER_IRQ_PRIORITY);
        // 將核心定時器的中段處理程序加入task
        irqtask_run_preemptive();
        // 執行task裏頭的中段處理程序
        core_timer_enable();
        // 啟用core timer準備下一次的定時事件
    }
}
// do the except handle 印出三個資訊
void el0_sync_router(){
    unsigned long long spsr_el1;
    __asm__ __volatile__("mrs %0, SPSR_EL1\n\t" : "=r" (spsr_el1)); // EL1 configuration, spsr_el1[9:6]=4b0 to enable interrupt
    unsigned long long elr_el1;
    __asm__ __volatile__("mrs %0, ELR_EL1\n\t" : "=r" (elr_el1));   // ELR_EL1 holds the address if return to EL1
    unsigned long long esr_el1;
    __asm__ __volatile__("mrs %0, ESR_EL1\n\t" : "=r" (esr_el1));   // ESR_EL1 holds symdrome information of exception, to know why the exception happens.
    // 使用uart發送三個資訊
    uart_sendline("[Exception][el0_sync] spsr_el1 : 0x%x, elr_el1 : 0x%x, esr_el1 : 0x%x\n", spsr_el1, elr_el1, esr_el1);
}

void el0_irq_64_router(){
    // decouple the handler into irqtask queue
    // (1) https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf - Pg.113
    // (2) https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf - Pg.16
    if(*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) // from aux && from GPU0 -> uart exception
    // check interrupt from uart & GPU
    {
        if (*AUX_MU_IIR_REG & (1 << 1)) //有write interrupt
        {
            *AUX_MU_IER_REG &= ~(2);  // disable write interrupt
            irqtask_add(uart_w_irq_handler, UART_IRQ_PRIORITY);
            // 把這個寫中斷加入task queue
            irqtask_run_preemptive();
            // 立即執行queue中的task
        }
        else if (*AUX_MU_IIR_REG & (2 << 1)) // 有read interrupt
        {
            *AUX_MU_IER_REG &= ~(1);  // disable read interrupt
            irqtask_add(uart_r_irq_handler, UART_IRQ_PRIORITY);
            irqtask_run_preemptive();
        }
    }
    else if(*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ)  //from CNTPNS (core_timer) // A1 - setTimeout run in el1
    // 來自 core0 物理計時器的中斷
    {
        core_timer_disable();
        irqtask_add(core_timer_handler, TIMER_IRQ_PRIORITY);
        irqtask_run_preemptive();
        core_timer_enable();
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

int curr_task_priority = 9999;   // Small number has higher priority // 9999優先級極低

struct list_head *task_list; // task list pointer指向一個list head 
void irqtask_list_init()
{
    INIT_LIST_HEAD(task_list); // next prev指向自己
}


void irqtask_add(void *task_function,unsigned long long priority){
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
    el1_interrupt_disable(); // 位什麼要有這個???
    // enqueue the processing task to the event queue with sorting.
    list_for_each(curr, task_list)
    {
        if (((irqtask_t *)curr)->priority > the_task->priority)
        {
            list_add(&the_task->listhead, curr->prev);
            // the_task插入正確位置
            break;
        }
    }
    // if the priority is lowest
    if (list_is_head(curr, task_list))
    {
        list_add_tail(&the_task->listhead, task_list);
    }
    // unmask the interrupt line
    el1_interrupt_enable();
}

void irqtask_run_preemptive(){
    el1_interrupt_enable();
    // 若有更高優先極的中斷->要立即反應
    while (!list_empty(task_list))
    {
        // critical section protects new coming node
        el1_interrupt_disable();
        irqtask_t *the_task = (irqtask_t *)task_list->next;
        // Run new task (early return) if its priority is lower than the scheduled task.
        if (curr_task_priority <= the_task->priority)
        // curr_task_priority 比較優先，不會被搶占
        {
            el1_interrupt_enable();
            break;
            // 中斷循環
        }
        // curr_task_priority > the_task->priority
        // the_task比較優先，搶佔curr
        list_del_entry((struct list_head *)the_task);
        int prev_task_priority = curr_task_priority;
        curr_task_priority = the_task->priority;
        // 修改優先級時不能發生race condition

        el1_interrupt_enable();
        irqtask_run(the_task);
        // 執行task，並允許更高優先級的task搶占
        el1_interrupt_disable();

        curr_task_priority = prev_task_priority;
        // 修改優先級時不能發生race condition
        el1_interrupt_enable();
        free(the_task);
    }
}

void irqtask_run(irqtask_t* the_task)
{
    ((void (*)())the_task->task_function)();
    // 執行the_task 的 function
}

