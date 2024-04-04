#include "entry.h"
#include "mini_uart.h"

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
    uart_send_string(entry_error_type[type]);
    uart_send_string(", SPSR: 0x");
    uart_send_hex(spsr >> 32);
    uart_send_hex(spsr);
    uart_send_string(", ESR: 0x");
    uart_send_hex(esr >> 32);
    uart_send_hex(esr);
    uart_send_string(", ELR: 0x");
    uart_send_hex(elr >> 32);
    uart_send_hex(elr);
    uart_send_string("\n");
    while (1)
        ;
}
