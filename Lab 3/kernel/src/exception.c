#include "type.h"
#include "uart.h"
#include "asm.h"
#include "core_timer.h"
#include "irq.h"
#include "aux.h"
#include "list.h"
#include "memory.h"


void 
exception_l1_enable() {
    asm volatile("msr daifclr, 0xf");           // umask all DAIF, it's column D,A,I,F on the register PASATE 
}


void
exception_l1_disable()
{
    asm volatile("msr daifset, 0xf");           // mask all DAIF, it's column D,A,I,F on the register PASATE 
}


typedef void (*task_callback) ();

typedef struct irqtask
{
    struct list_head    listhead;
    uint64_t            running;
    uint64_t            priority;   // store priority (smaller number is more preemptive)
    task_callback       handler;       // task function pointer
} irqtask_t;

typedef irqtask_t* irqtask_ptr_t;


static LIST_HEAD(irqtask_queue);


static irqtask_ptr_t
irqtask_create(task_callback handler, uint64_t priority)
{
    irqtask_ptr_t task = (irqtask_ptr_t) malloc(sizeof(irqtask_t));
    INIT_LIST_HEAD(&task->listhead);
    task->handler = handler;
    task->priority = priority;
    task->running = 0;
    return task;
}


static void
irqtask_queue_add(irqtask_ptr_t task)
{
    if (task == 0) return; 
    list_head_ptr_t curr = (&irqtask_queue)->next;
    // small value has higher priority
    while (!list_is_head(curr, &irqtask_queue) && 
            ((irqtask_ptr_t) curr)->priority <= (task->priority))
    {
        curr = curr->next;
    }
    list_add(&task->listhead, curr->prev);
}


static uint32_t
irqtask_queue_not_running()
{
    if (list_empty(&irqtask_queue))
        return 0;
    irqtask_ptr_t first = (irqtask_ptr_t) (&irqtask_queue)->next;
    return !(first->running);
}


static void
irqtask_queue_run()
{
    while (irqtask_queue_not_running()) {
        irqtask_ptr_t first = (irqtask_ptr_t) (&irqtask_queue)->next;
        exception_l1_enable();
        first->running = 1;
        first->handler();
        exception_l1_disable();
        list_del_entry((list_head_ptr_t) first);
        // todo: free(first);
    }
}


static void 
show_exception_infomation()
{
    uint64_t spsr = asm_read_sysregister(spsr_el1);
    uint64_t elr  = asm_read_sysregister(elr_el1);
    uint64_t esr  = asm_read_sysregister(esr_el1);
    uart_str("spsr_el1: 0x"); uart_hexl(spsr); uart_endl();
    uart_str("elr_el1:  0x"); uart_hexl(elr); uart_endl();
    uart_str("esr_el1:  0x"); uart_hexl(esr); uart_endl();
}


void 
exception_invalid_handler()
{
    exception_l1_disable();
    uart_line("---- exception_invalid_handler ----");
    show_exception_infomation();
    exception_l1_enable();
}


void
exception_el0_sync_handler()
{
    exception_l1_disable();
    uart_line("---- exception_el0_sync_handler ----");
    show_exception_infomation();
    exception_l1_enable();
}


void
exception_el0_irq_handler()
{
    exception_l1_disable();
    uart_line("---- exception_el0_irq_handler ----");
    show_exception_infomation();
    exception_l1_enable();
}


void      
exception_el1_sync_handler()
{
    // exception_l1_disable();
    // uart_line("---- exception_el1_sync_handler ----");
    // _show_exception_infomation();
    // exception_l1_enable();
}


void
exception_el1_irq_handler()
{
    exception_l1_disable();    
    irqtask_ptr_t task = 0;
    if (*IRQ_PENDING1 & (1 << 29)) {            // UART IRQ
        if (*AUX_MU_IIR & 0x4) {                // Receiver holds valid bytes
            aux_clr_rx_interrupts();
            task = irqtask_create(mini_uart_rx_handler, 3);
        }
        else if (*AUX_MU_IIR & 0x2) {           // Transmit holding register empty
            aux_clr_tx_interrupts();
            task = irqtask_create(mini_uart_tx_handler, 3);
        }
    }
    else if (*CORE0_INTERRUPT_SOURCE & 0x2) {   // Core timer IRQ
        core_timer_disable();
        task = irqtask_create(core_timer_interrupt_handler, 1);
    }
    irqtask_queue_add(task);
    irqtask_queue_run();
    exception_l1_enable();
}



