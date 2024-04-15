#include "irq.h"
#include "entry.h"
#include "mini_uart.h"
#include "peripheral/mini_uart.h"
#include "peripheral/timer.h"
#include "timer.h"
#include "utils.h"

const char* entry_error_type[] = {
    "SYNC_INVALID_EL1t",   "IRQ_INVALID_EL1t",
    "FIQ_INVALID_EL1t",    "ERROR_INVALID_EL1t",

    "SYNC_INVALID_EL1h",   "IRQ_INVALID_EL1h",
    "FIQ_INVALID_EL1h",    "ERROR_INVALID_EL1h",

    "SYNC_INVALID_EL0_64", "IRQ_INVALID_EL0_64",
    "FIQ_INVALID_EL0_64",  "ERROR_INVALID_EL0_64",

    "SYNC_INVALID_EL0_32", "IRQ_INVALID_EL0_32",
    "FIQ_INVALID_EL0_32",  "ERROR_INVALID_EL0_32"};

void show_invalid_entry_message(int type,
                                unsigned long spsr,
                                unsigned long esr,
                                unsigned long elr)
{
    disable_all_exception();
    uart_send_string(entry_error_type[type]);
    uart_send_string(": ");
    // decode exception type (some, not all. See ARM DDI0487B_b chapter
    // D10.2.28)
    switch (esr >> 26) {
    case 0b000000:
        uart_send_string("Unknown");
        break;
    case 0b000001:
        uart_send_string("Trapped WFI/WFE");
        break;
    case 0b001110:
        uart_send_string("Illegal execution");
        break;
    case 0b010101:
        uart_send_string("System call");
        break;
    case 0b100000:
        uart_send_string("Instruction abort, lower EL");
        break;
    case 0b100001:
        uart_send_string("Instruction abort, same EL");
        break;
    case 0b100010:
        uart_send_string("Instruction alignment fault");
        break;
    case 0b100100:
        uart_send_string("Data abort, lower EL");
        break;
    case 0b100101:
        uart_send_string("Data abort, same EL");
        break;
    case 0b100110:
        uart_send_string("Stack alignment fault");
        break;
    case 0b101100:
        uart_send_string("Floating point");
        break;
    default:
        uart_send_string("Unknown");
        break;
    }
    // decode data abort cause
    if (esr >> 26 == 0b100100 || esr >> 26 == 0b100101) {
        uart_send_string(", ");
        switch ((esr >> 2) & 0x3) {
        case 0:
            uart_send_string("Address size fault");
            break;
        case 1:
            uart_send_string("Translation fault");
            break;
        case 2:
            uart_send_string("Access flag fault");
            break;
        case 3:
            uart_send_string("Permission fault");
            break;
        }
        switch (esr & 0x3) {
        case 0:
            uart_send_string(" at level 0");
            break;
        case 1:
            uart_send_string(" at level 1");
            break;
        case 2:
            uart_send_string(" at level 2");
            break;
        case 3:
            uart_send_string(" at level 3");
            break;
        }
    }

    // dump registers
    uart_send_string(":\n, SPSR: 0x");
    uart_send_hex(spsr >> 32);
    uart_send_hex(spsr);
    uart_send_string(", ESR: 0x");
    uart_send_hex(esr >> 32);
    uart_send_hex(esr);
    uart_send_string(", ELR: 0x");
    uart_send_hex(elr >> 32);
    uart_send_hex(elr);
    uart_send_string("\n");

    enable_all_exception();
}

void irq_handler(void)
{
    disable_all_exception();
    unsigned int irq_pending_1 = get32(IRQ_PENDING_1);
    unsigned int core_irq_source = get32(CORE0_IRQ_SOURCE);
    if (irq_pending_1 & IRQ_PENDING_1_AUX_INT) {
        unsigned int iir = get32(AUX_MU_IIR_REG);
        unsigned int int_id = iir & INT_ID_MASK;
        if (int_id == RX_INT) {
            clear_rx_interrupt();
            uart_rx_handle_irq();
        } else if (int_id == TX_INT) {
            clear_tx_interrupt();
            uart_tx_handle_irq();
        }
    } else if (core_irq_source & CNTPNSIRQ) {
        disable_core0_timer();
        core_timer_handle_irq();
    } else {
        uart_send_string("Unknown pending interrupt\n");
    }
    enable_all_exception();
}
