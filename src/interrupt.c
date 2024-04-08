#include "interrupt.h"

#include "task.h"
#include "timer.h"
#include "uart1.h"
#include "utli.h"

static unsigned int el0_timer_trigger_cnt = 0;
extern void set_core_timer_int(unsigned long long s);
extern void set_core_timer_int_sec(unsigned int s);
extern void core0_timer_interrupt_disable();
extern void core0_timer_interrupt_enable();
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
};

void el0_64_sync_interrupt_handler() {
  add_task(print_el1_sys_reg, 20);
  pop_task();
  // print_el1_sys_reg();
}

void el0_64_irq_interrupt_handler() {
  if (*CORE0_INT_SRC & CORE_INT_SRC_TIMER) {
    set_core_timer_int(get_clk_freq() / 100);
    // for going back to shell
    el0_timer_trigger_cnt++;
    if (el0_timer_trigger_cnt == 5) {
      el0_timer_trigger_cnt = 0;
      asm volatile(
          "mov	x1, 0x0;"
          "mov  sp, 0x40000;"
          "msr	spsr_el1, x1;"
          "msr	elr_el1,  x1;"
          "msr	esr_el1,  x1;"
          "b    shell_start");
    }
    core0_timer_interrupt_disable();
    add_task(el0_timer_interrupt_handler, TIMER_INT_PRIORITY);
  }
  pop_task();
}

void el1h_irq_interrupt_handler() {
  if (*CORE0_INT_SRC & CORE_INT_SRC_TIMER) {
    set_core_timer_int(get_clk_freq() / 100);
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