
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
    REGS_IRQ->irq_enable_1 = AUX_IRQ | SYS_TIMER_1;
}

void handle_irq() {
    U32 irq = REGS_IRQ->irq_pending_1;

    while (irq) {
        // has AUX interrupt
        if (irq & AUX_IRQ) {
            irq &= ~AUX_IRQ;
                
            // pg. 13 in the read write bit
            // 10: Receiver holds valid byte
            // so when we and the iir with 4 getting 4
            // mean the receiver holds valid byte
            while ((REGS_AUX->mu_iir & 4) == 4) {
                uart_handle_int();
            }
        }

        if (irq & SYS_TIMER_1) {
            irq &= ~SYS_TIMER_1;

            handle_timer_1();
        }
    }
}
