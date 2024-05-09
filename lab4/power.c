#include "include/power.h"
#include "include/uart.h"

void reboot() {
  uart_sendline("Kernel will reboot in 10 seconds...\n");
  *PM_WDOG = PM_WDOG_MAGIC | 500000;
  *PM_RSTC = PM_WDOG_MAGIC | PM_RSTC_FULLRST;
}

void abort_reboot() {
  uart_sendline("Reboot has been cancelled.\n");
  *PM_WDOG = PM_WDOG_MAGIC;
  *PM_RSTC = PM_WDOG_MAGIC | 0;
}