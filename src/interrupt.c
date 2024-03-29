#include "interrupt.h"

#include "peripherals/aux.h"
#include "uart1.h"
#include "utli.h"

static unsigned int el0_timer_trigger_cnt = 0;

extern void core_timer_handler(unsigned int s);
extern void core_timer_disable();

void invaild_exception_handler() { uart_puts("invaild exception handler!"); }

static void el0_timer_interrupt_handler() {
  uart_puts("asynchronous timer interrupt in EL0!");
  uart_puts("timer will be triggered 2s later.");
  print_timestamp();
  core_timer_handler(2);

  // for going back to shell
  el0_timer_trigger_cnt++;
  if (el0_timer_trigger_cnt == 5) {
    el0_timer_trigger_cnt = 0;
    core_timer_disable();
    asm volatile("b shell_start");
  }
};

static void el1_timer_interrupt_handler() {
  uart_puts("asynchronous timer interrupt in EL1!");
  uart_puts("timer will be triggered 1s later.");
  print_timestamp();
  core_timer_handler(1);
};

static void uart_TX_interrupt_handler() {}

static void uart_RX_interrupt_handler() {}

static void uart_interrupt_handler() {
  uart_puts("uart_interrupt_handler!");
  if (*AUX_MU_IIR &
      0x2)  // bit[2:1]=01: Transmit holding register empty (FIFO empty)
  {
    uart_TX_interrupt_handler();
  } else if (*AUX_MU_IIR & 0x4)  // bit[2:1]=10: Receiver holds valid byte
                                 // (FIFO hold at least 1 symbol)
  {
    uart_RX_interrupt_handler();
  }
}

void el0_64_irq_interrupt_handler() {
  if (*CORE0_INTERRUPT_SOURCE & CORE0_INT_SRC_TIMER) {
    el0_timer_interrupt_handler();
  }
}

void el1h_irq_interrupt_handler() {
  if (*CORE0_INTERRUPT_SOURCE & CORE0_INT_SRC_TIMER) {
    el1_timer_interrupt_handler();
  } else if (*IRQ_PENDING_1 &
             0x20000000) {  //  bit 29 : AUX intterupt (uart1_interrupt)
    uart_interrupt_handler();
  }
}
