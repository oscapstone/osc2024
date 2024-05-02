#include "../peripherals/mini_uart.h"
#include "timer.h"
#include "irq.h"
#include "task.h"
#include "alloc.h"

const char *entry_error_messages[] = {
	"SYNC_INVALID_EL1t",
	"IRQ_INVALID_EL1t",		
	"FIQ_INVALID_EL1t",		
	"ERROR_INVALID_EL1T",		

	"SYNC_INVALID_EL1h",		
	"IRQ_INVALID_EL1h",		
	"FIQ_INVALID_EL1h",		
	"ERROR_INVALID_EL1h",		

	"SYNC_INVALID_EL0_64",		
	"IRQ_INVALID_EL0_64",		
	"FIQ_INVALID_EL0_64",		
	"ERROR_INVALID_EL0_64",	

	"SYNC_INVALID_EL0_32",		
	"IRQ_INVALID_EL0_32",		
	"FIQ_INVALID_EL0_32",		
	"ERROR_INVALID_EL0_32"	
};

void show_invalid_entry_message(int type, unsigned long esr, unsigned long address) {
    uart_send_string((char *)entry_error_messages[type]);
    uart_send_string(", ESR: ");
    uart_send_uint(esr);
    uart_send_string(", address: ");
    uart_send_uint(address);
    uart_send_string("\r\n");
}

void handle_sync_el0_64(void) {
    unsigned long spsr_el1, elr_el1, esr_el1;

    asm volatile (
        "mrs %0, spsr_el1;"
        "mrs %1, elr_el1;"
        "mrs %2, esr_el1;"

        // Output operand.
        : "=r" (spsr_el1), "=r" (elr_el1), "=r" (esr_el1)
        :
        // Clobber list.
        : "x0", "x1", "x2"
    );

    uart_send_string("In handler: handle_sync_el0_64\r\n");
    uart_send_string("spsr_el1: ");
    uart_send_uint(spsr_el1);
    uart_send_string("\r\n");
    uart_send_string("elr_el1: ");
    uart_send_uint(elr_el1);
    uart_send_string("\r\n");
    /*
     * For assembly user code, it showed 0x56000000.
     * [31:26] 0b010101 -> SVC instruction execution in AArch64 state.
     */

    uart_send_string("esr_el1: ");
    uart_send_uint(esr_el1);
    uart_send_string("\r\n");

    //set_timer_expire(3);
}

void handle_irq_el0_64(void) {
    unsigned long spsr_el1, elr_el1, esr_el1, sp_el0;

    asm volatile (
        "mrs %0, spsr_el1;"
        "mrs %1, elr_el1;"
        "mrs %2, esr_el1;"
        "mrs %3, sp_el0;"

        // Output operand.
        : "=r" (spsr_el1), "=r" (elr_el1), "=r" (esr_el1), "=r" (sp_el0)
        :
        // Clobber list.
        : "x0", "x1", "x2"
    );

    uart_send_string("In handler: handle_irq_el0_64\r\n");
    uart_send_string("spsr_el1: ");
    uart_send_uint(spsr_el1);
    uart_send_string("\r\n");
    uart_send_string("elr_el1: ");
    uart_send_uint(elr_el1);
    uart_send_string("\r\n");
    uart_send_string("sp_el0: ");
    uart_send_uint(sp_el0);
    uart_send_string("\r\n");
    /*
     * For assembly user code, it showed 0x56000000.
     * [31:26] 0b010101 -> SVC instruction execution in AArch64 state.
     */

    uart_send_string("esr_el1: ");
    uart_send_uint(esr_el1);
    uart_send_string("\r\n");

    // For checking whether timer caused the interrupt.
    unsigned long cntp_ctl;
    static int cnt = 0;
    asm volatile("mrs %0, cntp_ctl_el0" : "=r" (cntp_ctl));

    // Timer interrupt is pending
    if (cntp_ctl & (1 << 2)) {
        uart_send_string("Timer interrupt detected ");
        uart_send_uint(cnt++);
        uart_send_string(".\r\n");

        // Reset the timer.
        set_timer_expire(2);
        
        
    }
}

void handle_irq_el1h(uint64_t sp) {
    disable_el1_interrupt();

    uart_send_string("\r\nIn handler: handle_irq_el1h\r\n");
    uart_send_string("SP: ");
    uart_send_uint(sp);
    uart_send_string("\r\n");

    task* t = (task *)simple_malloc(sizeof(task));
    t->sp = sp;
    // On manual P.9
    // If AUX_IRQ[0] is set to 1, mini UART has an interrupt pending.
    if (*AUX_IRQ & 0x1) {
        uart_send_string("Uart interrupt\r\n");
        
        t->intr_func = handle_uart_interrupt;
        t->arg = NULL;
        t->priority = 1;
        enqueue_task(t);
        enable_el1_interrupt();
        
        // handle_uart_interrupt();
    }

    // Check whether timer expires.
    int timer_intr;
    asm volatile (
        "mrs %0, cntp_ctl_el0;"

        : "=r" (timer_intr)
        :
        :
    );

    // cntp_ctl_el0[2] = 1 -> timer condition met. This bit is read-only, cannot be modified.
    // The bit will be cleared when a new expired time is set.
    if (timer_intr & 0x4) {
        uart_send_string("Timer interrupt\r\n");
        clear_timer_intr();
        // t->intr_func = handle_timer_intr;
        // t->arg = NULL;
        // t->priority = 0;
 
        // enqueue_task(t);
        // enable_el1_interrupt();
        // execute_task(t);
        
        handle_timer_intr(NULL);
    }



}

void handle_sync_el1h(void) {
    
    //disable_el1_interrupt();
    uart_send_string("In handler: handle_sync_el1h\r\n");
    unsigned long spsr_el1, elr_el1, esr_el1, sp_el0;

    asm volatile (
        "mrs %0, spsr_el1;"
        "mrs %1, elr_el1;"
        "mrs %2, esr_el1;"
        "mrs %3, sp_el0;"

        // Output operand.
        : "=r" (spsr_el1), "=r" (elr_el1), "=r" (esr_el1), "=r" (sp_el0)
        :
        // Clobber list.
        : "x0", "x1", "x2"
    );

    uart_send_string("spsr_el1: ");
    uart_send_uint(spsr_el1);
    uart_send_string("\r\n");
    uart_send_string("elr_el1: ");
    uart_send_uint(elr_el1);
    uart_send_string("\r\n");
    uart_send_string("esr_el1: ");
    uart_send_uint(esr_el1);
    uart_send_string("\r\n");
    uart_send_string("sp_el0: ");
    uart_send_uint(sp_el0);
    uart_send_string("\r\n");
    //enable_el1_interrupt();
    
}