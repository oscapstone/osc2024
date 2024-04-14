#include "int/exception.hpp"

#include "board/mini-uart.hpp"
#include "int/interrupt.hpp"
#include "shell.hpp"

const char* ExceptionFrom[] = {
    "Current Exception level with SP_EL0.",
    "Current Exception level with SP_ELx, x>0.",
    "Lower Exception level, using AArch64",
    "Lower Exception level, using AArch32",
};
const char* ExceptionType[] = {
    "Synchronous",
    "IRQ or vIRQ",
    "FIQ or vFIQ",
    "SError or vSError",
};

void print_exception(ExceptionContext* context, int type) {
  mini_uart_printf_sync("(%d) %s: %s\n", type, ExceptionType[type % 4],
                        ExceptionFrom[type / 4]);
  mini_uart_printf_sync("SPSR_EL1: %032lb\n", context->spsr_el1);
  mini_uart_printf_sync("ELR_EL1 : 0x%lx\n", context->elr_el1);
  mini_uart_printf_sync("ESR_EL1 : %032lb\n", context->esr_el1);

  enable_interrupt();
  shell();
}
