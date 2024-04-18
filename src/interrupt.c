#include "interrupt.h"

#include "multitask.h"
#include "task.h"
#include "timer.h"
#include "uart1.h"
#include "utli.h"

static uint32_t lock_cnt = 0;

void invaild_exception_handler() { uart_puts("invaild exception handler!"); }

static void el0_timer_interrupt_handler() {
  uart_puts("EL0 timer interrupt");
  print_timestamp();
  uart_write('\n');
  set_core_timer_int_sec(2);
  core0_timer_interrupt_enable();
};

static void el1_timer_interrupt_handler() {
  timer_event_pop();
  core0_timer_interrupt_enable();
  schedule();
};

void el0_64_sync_interrupt_handler() {
  add_task(print_el1_sys_reg, SW_INT_PRIORITY);
  pop_task();
}

void el0_64_irq_interrupt_handler() {
  if (*CORE0_INT_SRC & CORE_INT_SRC_TIMER) {
    set_core_timer_int(get_clk_freq() >> 5);
    core0_timer_interrupt_disable();
    add_task(el0_timer_interrupt_handler, TIMER_INT_PRIORITY);
  }
  pop_task();
}

void el1h_irq_interrupt_handler() {
  if (*CORE0_INT_SRC & CORE_INT_SRC_TIMER) {
    set_core_timer_int(get_clk_freq() >> 5);
    core0_timer_interrupt_disable();
    add_task(el1_timer_interrupt_handler, TIMER_INT_PRIORITY);
  }
  if ((*CORE0_INT_SRC & CORE_INT_SRC_GPU) &&       //  (uart1_interrupt)
      (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT)) {  //  bit 29 : AUX interrupt
    uart_interrupt_handler();
  }
  pop_task();
}

void fake_long_handler() {
  wait_usec(3000000);
  uart_puts("fake long interrupt handler finish");
}

void OS_enter_critical() {
  disable_interrupt();
  lock_cnt++;
}

void OS_exit_critical() {
  lock_cnt--;
  if (lock_cnt == 0) {
    enable_interrupt();
  }
}