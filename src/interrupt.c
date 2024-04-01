#include "interrupt.h"

#include "timer.h"
#include "uart1.h"
#include "utli.h"

static unsigned int el0_timer_trigger_cnt = 0;

extern void core_timer_handler(unsigned int s);
extern void core0_timer_interrupt_disable();

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
    core0_timer_interrupt_disable();
    asm volatile("b shell_start");
  }
};

static void el1_timer_interrupt_handler() { timer_event_pop(); };

void el0_64_irq_interrupt_handler() {
  if (*CORE0_INT_SRC & CORE0_INT_SRC_TIMER) {
    el0_timer_interrupt_handler();
  }
}

void el1h_irq_interrupt_handler() {
  if (*CORE0_INT_SRC & CORE0_INT_SRC_TIMER) {
    el1_timer_interrupt_handler();
  } else if ((*CORE0_INT_SRC & CORE0_INT_SRC_GPU) &&
             (*IRQ_PENDING_1 &
              IRQ_PENDING_1_AUX_INT)) {  //  bit 29 : AUX intterupt
                                         //  (uart1_interrupt)
    uart_interrupt_handler();
  }
}
