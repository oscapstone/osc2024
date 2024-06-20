#include "peripheral/pm.h"
#include "mini_uart.h"
#include "utils.h"

int EXIT = 0;

void reset(unsigned int tick)
{
    uart_send_string("rebooting...\n");
    EXIT = 1;
    put32(PM_RSTC, PM_PASSWORD | 0x20);  // full reset
    put32(PM_WDOG, PM_PASSWORD | tick);  // number of watchdog tick
}

void cancel_reset(void)
{
    uart_send_string("reboot canceled\n");
    EXIT = 0;
    put32(PM_RSTC, PM_PASSWORD | 0);  // cancel reset
    put32(PM_WDOG, PM_PASSWORD | 0);  // number of watchdog tick
}
