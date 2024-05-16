#include "board/pm.hpp"

#include "board/mini-uart.hpp"
#include "mmio.hpp"
#include "util.hpp"

void reboot() {
  mini_uart_puts("rebooting .");
  reset(0x50);
  for (;;) {
    mini_uart_putc('.');
  }
}

void reset(int tick) {                 // reboot after watchdog timer expire
  set32(PM_RSTC, PM_PASSWORD | 0x20);  // full reset
  set32(PM_WDOG, PM_PASSWORD | tick);  // number of watchdog tick
}

void cancel_reset() {
  set32(PM_RSTC, PM_PASSWORD | 0);  // cancel reset
  set32(PM_WDOG, PM_PASSWORD | 0);  // number of watchdog tick
}
