#include "board/pm.hpp"

#include "int/interrupt.hpp"
#include "int/timer.hpp"
#include "io.hpp"
#include "util.hpp"

void reboot() {
  reboot(freq_of_timer);
}

void reboot(int tick) {
  disable_interrupt();
  kputs_sync("rebooting .");
  reset(0x50 + tick);
  for (;;) {
    kputc_sync('.');
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
