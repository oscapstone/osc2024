#include "board/timer.hpp"

#include "board/mini-uart.hpp"
#include "util.hpp"

uint64_t freq_of_timer, boot_timer_tick;
bool show_timer = true;

uint64_t get_timetick() {
  return read_sysreg(CNTPCT_EL0);
}

timeval tick2timeval(uint64_t tick) {
  uint32_t sec = tick / freq_of_timer;
  uint32_t usec = tick % freq_of_timer * (int)1e6 / freq_of_timer;
  return {sec, usec};
}

void core_timer_enable() {
  write_sysreg(CNTP_CTL_EL0, 1);
  freq_of_timer = read_sysreg(CNTFRQ_EL0);
  boot_timer_tick = get_timetick();
  set32(CORE0_TIMER_IRQ_CTRL, 2);
}

void set_core_timer(int sec) {
  write_sysreg(CNTP_TVAL_EL0, sec * freq_of_timer);
}

timeval get_boot_time() {
  return tick2timeval(get_timetick() - boot_timer_tick);
}

void timer_handler() {
  set_core_timer(2);
  if (show_timer)
    mini_uart_printf("[" PRTval "] timer interrupt\n", FTval(get_boot_time()));
}
