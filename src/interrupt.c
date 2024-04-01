#include "interrupt.h"

#include "task.h"
#include "timer.h"
#include "uart1.h"
#include "utli.h"

static unsigned int el0_timer_trigger_cnt = 0;

extern void core_timer_handler(unsigned int s);
extern void core0_timer_interrupt_disable();
extern void core0_timer_interrupt_enable();
void invaild_exception_handler() { uart_puts("invaild exception handler!"); }

static void el0_timer_interrupt_handler() {
  uart_puts("EL0 timer interrupt");
  print_timestamp();
  uart_write('\n');
  core_timer_handler(2);

  // for going back to shell
  el0_timer_trigger_cnt++;
  if (el0_timer_trigger_cnt == 5) {
    el0_timer_trigger_cnt = 0;
    // asm volatile("b shell_start");
    asm volatile(
        "mov	x1, 0x0;"
        "mov  sp, 0x40000;"
        "msr	spsr_el1, x1;"
        "msr	elr_el1,  x1;"
        "msr	esr_el1,  x1;"
        "b    shell_start"  // exception return
    );
  }
};

static void el1_timer_interrupt_handler() { timer_event_pop(); };

void el0_64_irq_interrupt_handler() {
  if (*CORE0_INT_SRC & CORE0_INT_SRC_TIMER) {
    core0_timer_interrupt_disable();
    el0_timer_interrupt_handler();
    core0_timer_interrupt_enable();
  }
}

void el1h_irq_interrupt_handler() {
  if (*CORE0_INT_SRC & CORE0_INT_SRC_TIMER) {
    el1_timer_interrupt_handler();

    // core0_timer_interrupt_disable();
    // add_task(el1_timer_interrupt_handler, TIMER_INT_PRIORITY);
    // core0_timer_interrupt_enable();
    //  pop_task();
    //
  }
  if ((*CORE0_INT_SRC & CORE0_INT_SRC_GPU) &&
      (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT)) {  //  bit 29 : AUX intterupt
                                                   //  (uart1_interrupt)
    uart_interrupt_handler();
    // add_task(uart_interrupt_handler, UART_INT_PRIORITY);
  }
  pop_task();
}
