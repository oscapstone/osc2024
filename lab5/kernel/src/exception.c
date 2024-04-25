#include "peripherals/rpi_irq.h"
#include "peripherals/mini_uart.h"
#include "exception.h"
#include "mini_uart.h"
#include "utils.h"
#include "timer.h"
#include "memory.h"
#include "sched.h"

int                 curr_task_priority = 9999; // Small number has higher priority
struct              list_head *irqtask_list;
extern list_head_t  *run_queue;
extern int          done_sched_init;


static unsigned long long lock_count = 0;

void print_currentEL(){
    unsigned long long currentEL;
    __asm__ __volatile__("mrs %0, currentEL\n\t" : "=r"(currentEL));
    
    switch (currentEL) {
        case 0:
            uart_puts("Current EL: EL0\r\n");
            break;
        case 4:
            uart_puts("Current EL: EL1\r\n");
            break;
        case 8:
            uart_puts("Current EL: EL2\r\n");
            break;
        case 12:
            uart_puts("Current EL: EL3\r\n");
            break;
        default:
            uart_puts("Current EL: UNKNOWN\r\n");
            break;
    }
}

// DAIF, Interrupt Mask Bits
void el1_interrupt_enable() {
    __asm__ __volatile__("msr daifclr, 0xf"); // umask all DAIF
}

void el1_interrupt_disable() {
    __asm__ __volatile__("msr daifset, 0xf"); // mask all DAIF
}

void el1h_irq_router() {
    lock();
    if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) {
        if (*AUX_MU_IER_REG & 2) {
            *AUX_MU_IER_REG &= ~(2); // disable write interrupt
            irqtask_add(uart_write_irq_handler, UART_IRQ_PRIORITY);
            unlock();
            irqtask_run_preemptive(); // run the queued task before returning to the program.
        }
        else if (*AUX_MU_IER_REG & 1) {
            *AUX_MU_IER_REG &= ~(1);
            irqtask_add(uart_read_irq_handler, UART_IRQ_PRIORITY);
            unlock();
            irqtask_run_preemptive();
        }
    }
    else if (*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ) {
        core_timer_disable();
        irqtask_add(core_timer_handler, TIMER_IRQ_PRIORITY);
        unlock();
        irqtask_run_preemptive();
        core_timer_enable();
        // if (done_sched_init) schedule();
    }   
    else {
        uart_puts("UNKNOWN el1h_irq_router\r\n");
    }
}

void el0_sync_router() {
    // Print the content of spsr_el1, elr_el1, and esr_el1 in the exception handler.
    unsigned long long spsr_el1;
    unsigned long long elr_el1;
    unsigned long long esr_el1;
    __asm__ __volatile__("mrs %0, SPSR_EL1\n\t" : "=r"(spsr_el1)); // EL1 configuration, spsr_el1[9:6]=4b0 to enable interrupt
    __asm__ __volatile__("mrs %0, ELR_EL1\n\t"  : "=r"(elr_el1)); // ELR_EL1 holds the address if return to EL1
    __asm__ __volatile__("mrs %0, ESR_EL1\n\t"  : "=r"(esr_el1)); // ESR_EL1 holds symdrome information of exception, to know why the exception happens.

    char buf[VSPRINT_MAX_BUF_SIZE];
    sprintf(buf, "[Exception][el0_sync] spsr_el1: 0x%x | elr_el1: 0x%x | esr_el1 : 0x%x\r\n", spsr_el1, elr_el1, esr_el1);
    uart_puts(buf);
}

void el0_irq_64_router() {
    uart_puts("el0_irq_64_router\r\n");
}

void invalid_exception_router(unsigned long long x0) {
    uart_puts("Invalid exception : 0x%x\r\n",x0);
}

void lock() {
    el1_interrupt_disable();
    lock_count++;
}

void unlock() {
    lock_count--;
    if (lock_count == 0)
        el1_interrupt_enable();
}

void irqtask_list_init() {
    irqtask_list = malloc(sizeof(irqtask_t));
    INIT_LIST_HEAD(irqtask_list);
}

void irqtask_add(void *task_function, unsigned long long priority) {
    irqtask_t *irq_task = malloc(sizeof(irqtask_t));
    irq_task->priority = priority;
    irq_task->task_function = task_function;
    INIT_LIST_HEAD(&(irq_task->listhead));

    struct list_head *ptr;
    list_for_each(ptr, irqtask_list) {
        if (((irqtask_t *)ptr)->priority > irq_task->priority) {
            list_add(&(irq_task->listhead), ptr->prev);
            break;
        }
    }
    // Lowest Priority
    if (list_is_head(ptr, irqtask_list)) {
        list_add_tail(&(irq_task->listhead), irqtask_list);
    }
}

void irqtask_run(irqtask_t *task) {
    ((void (*)())task->task_function)();
}

void irqtask_run_preemptive() {
    while (!list_empty(irqtask_list)) {
        lock();
        irqtask_t *irq_task = (irqtask_t *)irqtask_list->next;

        // uart_puts("Current task prio: ");
        // put_int(curr_task_priority);
        // uart_puts("\r\n");

        if (curr_task_priority <= irq_task->priority) {
            // uart_puts("unlock\r\n");
            unlock();
            break;
        }
        
        // struct list_head* curr;
        // uart_puts("DEBUG: ");
        // list_for_each(curr, irqtask_list) {
        //     put_int(((irqtask_t *)curr)->priority);
        //     uart_puts(" ");
        // }
        // uart_puts("\r\n");

        list_del_entry((struct list_head *)irq_task);
        int prev_task_priority = curr_task_priority;
        curr_task_priority = irq_task->priority;
        
        unlock();
        irqtask_run(irq_task);
        lock();

        curr_task_priority = prev_task_priority;
        unlock();
    }
}
