#include "peripherals/irq.h"
#include "peripherals/mini_uart.h"

#include "irq.h"
#include "io.h"

extern void enable_irq();
extern void disable_irq();
extern void irq_timer_handler();

static char *entry_error_messages[] = {
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

static void show_debug_msg(int type, unsigned long spsr, unsigned long elr, unsigned long esr)
{
    printf("\r\n");
    printf(entry_error_messages[type]);
    printf(", SPSR: ");
    printf_hex(spsr);
    printf(", ELR: ");
    printf_hex(elr);
    printf(", ESR: ");
    printf_hex(esr);
}

void el1_irq_entry(int type, unsigned long spsr, unsigned long elr, unsigned long esr)
{
    disable_irq();

    unsigned int irq = *CORE0_IRQ_SOURCE;
    switch(irq)
    {
        case SYSTEM_TIMER_IRQ_1:
            irq_timer_handler();
            break;
        default:
            printf("\r\nUnknown irq: ");
            printf_hex(irq);
            break;
    }
    enable_irq();
}

void default_exception_entry(int type, unsigned long spsr, unsigned long elr, unsigned long esr)
{
    
    show_debug_msg(type, spsr, elr, esr);
    while(1);
}

void el0_irq_entry(int type, unsigned long spsr, unsigned long elr, unsigned long esr)
{
    disable_irq();
    show_debug_msg(type, spsr, elr, esr);
    enable_irq();
}

void svc_exception_entry(int type, unsigned long spsr, unsigned long elr, unsigned long esr)
{
    disable_irq();
    show_debug_msg(type, spsr, elr, esr);
    enable_irq();
}
