
#include "base.h"
#include "irq.h"
#include "auxRegs.h"
#include "utils/printf.h"
#include "io/uart.h"
#include "peripherals/timer.h"
#include "utils/utils.h"
#include "arm/arm.h"

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
    U32 irq = REGS_IRQ->irq_pending_1;
    BOOL core_0_irq = REGS_ARM_CORE->irq_source[0];

    while (irq) {
        // has AUX interrupt and uart pending
        BOOL uart = (REGS_AUX->mu_iir & 0x1) == 0; // pg. 13 interrupt pending bit
        if ((irq & AUX_IRQ) && uart) {
            irq &= ~AUX_IRQ;

            uart_handle_int();
        }

        // has timer interrupt
        if (irq & SYS_TIMER_1) {
            irq &= ~SYS_TIMER_1;

            handle_timer_1();
        }
        if (irq & SYS_TIMER_3) {
            irq &= ~SYS_TIMER_3;

            timer_sys_timer_3_handler();
        }
    }

    // core 0 timer interrupt
    if (core_0_irq & 0x2) {
        timer_core_timer_0_handler();
    }
}

// ARMv8 pg. 254 DAIF, mask and unmask the bits (2 = irq)
void enable_interrupt() { asm volatile("msr DAIFClr, 0xf"); }
void disable_interrupt() { asm volatile("msr DAIFSet, 0xf"); }

U64 irq_disable() {
    U64 flag = utils_read_sysreg(DAIF);
    disable_interrupt();
    return flag;
}

void irq_restore(U64 flag) {
    utils_write_sysreg(DAIF, flag);
}

