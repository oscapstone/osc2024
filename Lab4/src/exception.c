#include "bcm2837/rpi_irq.h"
#include "bcm2837/rpi_uart1.h"
#include "uart.h"
#include "exception.h"
#include "timer.h"
#include "memory.h"

// DAIF, Interrupt Mask Bits
void enable_irq(){
    __asm__ __volatile__("msr daifclr, 0xf"); // umask all DAIF
}

void disable_irq(){
    __asm__ __volatile__("msr daifset, 0xf"); // mask all DAIF
}

void irq_router(){
    // bcm2835 - Pg.113
    // bcm2836 - Pg.16
    if(*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) // from aux && from GPU0 -> uart exception
    {
        if (*AUX_MU_IIR_REG & (1 << 1))
        {
            *AUX_MU_IER_REG &= ~(2);  // disable write interrupt
            add_task_list(uart_w_irq_handler, UART_PRIORITY);
            preemption(); // run the queued task before returning to the program.
        }
        else if (*AUX_MU_IIR_REG & (2 << 1))
        {
            *AUX_MU_IER_REG &= ~(1);  // disable read interrupt
            add_task_list(uart_r_irq_handler, UART_PRIORITY);
            preemption();
        }
    }
    else if(*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ)  //from CNTPNS (core_timer) // A1 - setTimeout run in el1
    {
        core_timer_disable();
        add_task_list(core_timer_handler, TIMER_PRIORITY);
        preemption();
        core_timer_enable();
    }
}

void svc_router(){
    unsigned long long spsr_el1;
    __asm__ __volatile__("mrs %0, SPSR_EL1\n\t" : "=r" (spsr_el1)); // EL1 configuration, spsr_el1[9:6]=4b0 to enable interrupt
    unsigned long long elr_el1;
    __asm__ __volatile__("mrs %0, ELR_EL1\n\t" : "=r" (elr_el1));   // ELR_EL1 holds the address if return to EL1
    unsigned long long esr_el1;
    __asm__ __volatile__("mrs %0, ESR_EL1\n\t" : "=r" (esr_el1));   // ESR_EL1 holds symdrome information of exception, to know why the exception happens.
    uart_sendline("spsr_el1 : 0x%x, elr_el1 : 0x%x, esr_el1 : 0x%x\n", spsr_el1, elr_el1, esr_el1);
}

void invalid_exception_router(unsigned long long x0){
    uart_sendline("%x: invalid exception\n",x0 );

    while(1){}
}

int cur_priority = 9;

struct list_head* task_list;

void task_list_init(){
    INIT_LIST_HEAD(task_list);
}

// add new task to task list
void add_task_list(void* callback, unsigned long long priority){
    irqtask_t* cur_task = s_allocator(sizeof(irqtask_t));

    cur_task->priority = priority;
    cur_task->callback = callback;
    INIT_LIST_HEAD(&cur_task->listhead);

    struct list_head* ptr;

    disable_irq();

    list_for_each(ptr, task_list){
        if (((irqtask_t*)ptr)->priority > cur_task->priority){
            list_add(&cur_task->listhead, ptr->prev);
            break;
        }
    }

    if(list_is_head(ptr, task_list)){
        list_add_tail(&cur_task->listhead, task_list);
    }

    enable_irq();
}

// decide preemtion or not
void preemption(){
    enable_irq();
    while(!list_empty(task_list)){
        disable_irq();
        irqtask_t *first_task = (irqtask_t *)task_list->next;

        // process executing now has highest priority
        if(cur_priority <= first_task->priority){
            enable_irq();
            break;
        }

        // executing first task in the task bc its priority is higher
        list_del_entry((struct list_head*) first_task);
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
void run_task(irqtask_t* task){
    ((void (*)())task->callback)();
}