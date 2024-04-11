#include "bcm2837/rpi_irq.h"
#include "bcm2837/rpi_uart1.h"
#include "uart1.h"
#include "exception.h"
#include "timer.h"
#include "heap.h"

// DAIF, Interrupt Mask Bits
// DAIF: https://developer.arm.com/documentation/ddi0487/aa/?lang=en (P.236)
// 這段程式碼是用於啟用和禁用在異常層級1（EL1）的中斷

/*這個函式通過清除 DAIF 寄存器的值（將 DAIF 設為0），從而解除對中斷的屏蔽->也就是做enable interrupt
這表示在這個函式執行之後，系統將能夠響應中斷。*/
void el1_interrupt_enable(){
    __asm__ __volatile__("msr daifclr, 0xf"); // umask all DAIF (P.255) // Used to clear any or all of DAIF to 0 
}; 

/*這個函式設置 DAIF 寄存器的所有位（將 DAIF 設為0xf），這樣會屏蔽所有中斷。
當這個函式執行後，系統將不會響應中斷。*/
void el1_interrupt_disable(){
    __asm__ __volatile__("msr daifset, 0xf"); // mask all DAIF (P.255) // Used to set any or all of DAIF to 1
}

/*它是在異常層級1（EL1）用來處理硬體interrupt的路由函式。
這個函式會檢查哪些interrupt是待處理狀態，並根據interrupt的來源執行相應的處理程序。
*/
/*確定中斷來源：uart or timer*/
void el1h_irq_router(){
    // decouple the handler into irqtask queue
    // (1) https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf - Pg.113
    // (2) https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf - Pg.16

    /*
    1. 關於*IRQ_PENDING_1、IRQ_PENDING_1_AUX_INT，這兩個是與週邊設備的interrupt有關，比如UART讀寫
    2. 關於*CORE0_INTERRUPT_SOURCE、INTERRUPT_SOURCE_GPU則是與GPU的interrupt有關(set core timer就是GPU interrupt的一種)
    */
    
    if(*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) // from aux && from GPU0 -> uart exception
    {
        if (*AUX_MU_IIR_REG & (0b01 << 1))
        {
            *AUX_MU_IER_REG &= ~(2);  // disable write interrupt

            /*將uart_w_irq_handler的優先級設為1
            (UART_IRQ_PRIORITY=1)
            然後丟進去irq_task_list中 */
            irqtask_add(uart_w_irq_handler, UART_IRQ_PRIORITY);

            /*使用這個函式直接強制執行UART r/w interrupt
              run the queued task before returning to the program.*/
            irqtask_run_preemptive(); 
        }
        else if (*AUX_MU_IIR_REG & (0b10 << 1))
        {
            *AUX_MU_IER_REG &= ~(1);  // disable read interrupt
            irqtask_add(uart_r_irq_handler, UART_IRQ_PRIORITY);
            irqtask_run_preemptive();
        }
    }
    else if(*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ){  
    // 若 INTERRUPT_SOURCE_CNTPNSIRQ[bit 1]=1, 則IRQ enable
    //from CNTPNS (core_timer) // A1 - setTimeout run in el1
        core_timer_disable();
        irqtask_add(core_timer_handler, TIMER_IRQ_PRIORITY);  // #define TIMER_IRQ_PRIORITY 0
        irqtask_run_preemptive(); // 在這裡run callback function (ex: uart_sendline, timer_set2sAlert)
        core_timer_enable();
    }
}
// 在user mode的interrupt處理是使用el0_irq_64_router
// 像是處理uart的i/o(read/write)、setTimeout cpu都是呼叫這個irq
/*確定中斷來源：uart or timer*/
void el0_irq_64_router(){
    // decouple the handler into irqtask queue
    // (1) https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf - Pg.113
    // (2) https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf - Pg.16
    if(*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) 
    // Interrupt來自：UART r/w interrupt
    // from aux && from GPU0 -> uart exception
    {
        if (*AUX_MU_IIR_REG & (0b01 << 1)) // *AUX_MU_IIR_REG & 0b10：若AUX_MU_IIR_REG[bit1=1]:代表有uart write interrupt
        {
            *AUX_MU_IER_REG &= ~(2);  // disable write interrupt：暫時不讓其他東西干擾它處理這次的interrupt
            /*將uart_w_irq_handler的優先級設為1
            (UART_IRQ_PRIORITY=1)
            然後丟進去irq_task_list中 */
            irqtask_add(uart_w_irq_handler, UART_IRQ_PRIORITY);
            // 使用這個函式直接強制執行UART r/w interrupt
            irqtask_run_preemptive();
        }
        else if (*AUX_MU_IIR_REG & (0b10 << 1)) // *AUX_MU_IIR_REG & 0b100：若AUX_MU_IIR_REG[bit1=0,bit2=1]:代表有uart read interrupt
        {
            *AUX_MU_IER_REG &= ~(1);  // disable read interrupt
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
    }
}

// el1->el0: print spsr_el1, elr_el1, esr_el1 on the console (after trigger SVC in user_proc.S)
/*spsr_el1: https://developer.arm.com/documentation/ddi0595/2021-12/AArch64-Registers/SPSR-EL1--Saved-Program-Status-Register--EL1-
            https://developer.arm.com/documentation/ddi0487/aa/?lang=en : P.281
* elr_el1: https://developer.arm.com/documentation/ddi0595/2021-12/AArch64-Registers/ELR-EL1--Exception-Link-Register--EL1-
* esr_el1: https://developer.arm.com/documentation/ddi0595/2021-12/AArch64-Registers/ESR-EL1--Exception-Syndrome-Register--EL1-
*/
// spsr_el1: <9:6>=D A I F
void el0_sync_router(){
    // 底下asm的inline assembly code的用意都是將系統暫存器中的
    // SPSR_EL1, ELR_EL1, ESR_EL1這三個由cpu core填進去的異常資訊存入c語言中的變數中（以下小寫的變數）
    // 目的就是打印出來！！！
    unsigned long long spsr_el1;
    /* mrs x0<-SPSR_EL1
       str x0->[sp, #40]
    */
    __asm__ __volatile__("mrs %0, SPSR_EL1\n\t" : "=r" (spsr_el1)); // EL1 configuration, spsr_el1[9:6]=4b0 to enable interrupt(Exception not masked) // MRS <Xt>, SPSR_EL1 ; Read SPSR_EL1 into Xt
    unsigned long long elr_el1;
    /*mrs  x0<-ELR_EL1
      str  x0->[sp, #32]
    */
    __asm__ __volatile__("mrs %0, ELR_EL1\n\t" : "=r" (elr_el1));   // ELR_EL1 holds the address if return to EL1
    unsigned long long esr_el1;
    /*mrs  x0<-ESR_EL1
      str  x0->[sp, #24]
    */
    __asm__ __volatile__("mrs %0, ESR_EL1\n\t" : "=r" (esr_el1));   // ESR_EL1 holds symdrome information of exception, to know why the exception happens.
    
    
    uart_sendline("[Exception][el0_sync] spsr_el1 : 0x%x, elr_el1 : 0x%x, esr_el1 : 0x%x\n", spsr_el1, elr_el1, esr_el1);
    
    // Print: esr_el1中的 ec（位於31 bits到26 bits，用於表示異常的類型，比如是由於一個不合法指令還是其他原因引起的異常）
	unsigned ec = (esr_el1 >> 26) & 0x3F; //0x3F = 0b111111(6)
    uart_sendline("EC : 0x%x\n", ec); // ec = 0x15 = 0b010101

    // Print: spsr_el1 的 DAIF
	unsigned daif_1 = (spsr_el1 >> 6) & 0xF; //0xF = 1111
    uart_sendline("DAIF : 0x%x\n", daif_1); //daif_1 = 0000 
    
}

// 處理invalid的exception
void invalid_exception_router(unsigned long long x0){ // x0從entry.S傳到這裡
    uart_sendline("invalid exception : 0x%x\r\n",x0);
    while(1);
}

// ------------------------------------------------------------------------------------------
// Advanced Exercise 2 - Concurrent I/O Devices Handling
/*
Preemption
Now, any interrupt handler can preempt the task’s execution, but the newly enqueued task still needs to wait for the currently running task’s completion.
It’d be better if the newly enqueued task with a higher priority can preempt the currently running task.
To achieve the preemption, the kernel can check the last executing task’s priority before returning to the previous interrupt handler.
If there are higher priority tasks, execute the highest priority task.
*/

int curr_task_priority = 9999;   // Small number has higher priority
// task_list的初始化
struct list_head *task_list;
void irqtask_list_init()
{
    INIT_LIST_HEAD(task_list);
}

// irqtask_add(uart_w_irq_handler, UART_IRQ_PRIORITY);
// irqtask_add(core_timer_handler, TIMER_IRQ_PRIORITY);  // #define TIMER_IRQ_PRIORITY 0
// UART_IRQ_PRIORITY = 1
void irqtask_add(void *task_function,unsigned long long priority){
    irqtask_t *the_task = kmalloc(sizeof(irqtask_t)); // free by irq_task_run_preemptive()

    // store all the related information into irqtask node
    // manually copy the device's buffer
    the_task->priority = priority;
    the_task->task_function = task_function;
    INIT_LIST_HEAD(&the_task->listhead);

    // add the timer_event into timer_event_list (sorted)
    // if the priorities are the same -> FIFO
    struct list_head *curr;

    // mask the device's interrupt line
    el1_interrupt_disable();
    // enqueue the processing task to the event queue with sorting.
    list_for_each(curr, task_list) // curr指針去遍歷task_list
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
    el1_interrupt_enable();
}

// Advanced Exercise 2 - Concurrent I/O Devices Handling
void irqtask_run_preemptive(){
    el1_interrupt_enable();
    while (!list_empty(task_list))
    {
        // critical section protects new coming node
        el1_interrupt_disable();
        // 指針the_task指向原本隊列中的next才是我們要run的task(head node沒東西)
        irqtask_t *the_task = (irqtask_t *)task_list->next; 
        // int curr_task_priority = 9999;
        // Run new task (early return) if its priority is lower than the scheduled task.
        /*因為一開始就宣告curr_task_priority為9999，所以不會有task等級比它高
        若curr_task_priority比*插隊*等待處理的interrupt task的priority還要小或是相等的話，
        el1_interrupt_enable=>不會讓它插隊處理，直接break。
        只要call這個preemptive函數就會優先讓這個插隊的task去做。
        */
        // int curr_task_priority = 9999;
        if (curr_task_priority <= the_task->priority)
        {
            el1_interrupt_enable();
            break;
        }
        // get the scheduled task and run it.
        list_del_entry((struct list_head *)the_task);// 先把the preemptive task從task_list中刪除
        int prev_task_priority = curr_task_priority; // prev_task_priority=9999
        curr_task_priority = the_task->priority; // curr_task_priority = 1

        el1_interrupt_enable();
        irqtask_run(the_task);// 執行the preemptive task
        el1_interrupt_disable();

        curr_task_priority = prev_task_priority; //恢復curr_task_priority=9999
        el1_interrupt_enable();
        free(the_task);
    }
}

void irqtask_run(irqtask_t* the_task)
{
    /*將 the_task->task_function 轉換為一個函數指針 void (*)()，
      這表示一個返回 void 且不接受參數的函數。
      (void (*)())：將 task_function 函數指針轉換為一個沒有參數且返回 void 的函數類型。
      ()：調用轉換後的函數。
    */
    // 調用 task_function 並執行的語法
    ((void (*)())the_task->task_function)();
}

