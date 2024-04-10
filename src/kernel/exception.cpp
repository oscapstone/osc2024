#include "exception.hpp"

#include "board/mini-uart.hpp"
#include "timer.hpp"

void print_exception(ExceptionContext* context, int type) {
  mini_uart_printf("Type    : %d %d\n", type / 4, type % 4);
  mini_uart_printf("SPSR_EL1: 0x%lx\n", context->spsr_el1);
  mini_uart_printf("ELR_EL1 : 0x%lx\n", context->elr_el1);
  mini_uart_printf("ESR_EL1 : 0x%lx\n", context->esr_el1);
}

void irq_handler(ExceptionContext* context, int type) {
  auto irq_source = get32(CORE0_IRQ_SOURCE);
  if ((irq_source & CNTPNSIRQ_INT) == CNTPNSIRQ_INT)
    timer_handler();
  if ((irq_source & GPU_INT) == GPU_INT)
    mini_uart_handler();
}
