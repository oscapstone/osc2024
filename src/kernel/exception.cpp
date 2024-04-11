#include "exception.hpp"

#include "board/mini-uart.hpp"
#include "board/pm.hpp"
#include "interrupt.hpp"
#include "timer.hpp"
#include "util.hpp"

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
  disable_interrupt();
  mini_uart_use_async(false);
  mini_uart_printf_sync("(%d) %s: %s\n", type, ExceptionType[type % 4],
                        ExceptionFrom[type / 4]);
  mini_uart_printf_sync("SPSR_EL1: %032lb\n", context->spsr_el1);
  mini_uart_printf_sync("ELR_EL1 : %032lb\n", context->elr_el1);
  mini_uart_printf_sync("ESR_EL1 : %032lb\n", context->esr_el1);
  reboot();
  prog_hang();
}

void irq_handler(ExceptionContext* context, int type) {
  auto irq_source = get32(CORE0_IRQ_SOURCE);
  if ((irq_source & CNTPNSIRQ_INT) == CNTPNSIRQ_INT)
    timer_handler();
  if ((irq_source & GPU_INT) == GPU_INT)
    mini_uart_handler();
}
