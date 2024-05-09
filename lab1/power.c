#include "power.h"
#include "uart.h"

void reboot() {
  uart_send("Rebooting in 10 sec...\n");
  *PM_WDOG = PM_WDOG_MAGIC | 500000;
  *PM_RSTC = PM_WDOG_MAGIC | PM_RSTC_FULLRST;
}

void cancel_reboot() {
  uart_send("Cancelling reboot...\n");
  *PM_WDOG = PM_WDOG_MAGIC;
  *PM_RSTC = PM_WDOG_MAGIC | 0;
}