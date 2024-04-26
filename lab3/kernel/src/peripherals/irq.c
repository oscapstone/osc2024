
#include "base.h"
#include "irq.h"
#include "auxRegs.h"
#include "utils/printf.h"
#include "io/uart.h"
#include "peripherals/timer.h"

const char entry_error_messages[16][32] = {
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

void show_invalid_entry_message(U32 type, U64 esr, U64 address, U64 spsr) {
    printf("ERROR exception: %s - %d, ESR: %X, Address: %X SPSR: %x\n", entry_error_messages[type], type, esr, address, spsr);
}

void enable_interrupt_controller() {
    // enable the interrupt
    // AUX: Mini uart interrupt
    // Sys timer 1: system timer 1
    REGS_IRQ->irq_enable_1 = AUX_IRQ | SYS_TIMER_1;
}

void handle_irq() {
    // disable when handling irq
    irq_disable();
    U32 irq = REGS_IRQ->irq_pending_1;

    while (irq) {
        // has AUX interrupt
        if (irq & AUX_IRQ) {
            irq &= ~AUX_IRQ;
               
            uart_handle_int();
        }

        // has timer interrupt
        if (irq & SYS_TIMER_1) {
            irq &= ~SYS_TIMER_1;

            handle_timer_1();
        }
    }
    // enable the interrupt again
    irq_enable();
}
