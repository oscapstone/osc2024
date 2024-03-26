#ifndef __SHELL_H
#define __SHELL_H

#include "mini_uart.h"
#include "utils.h"
#include "mailbox.h"
#include "m_string.h"
#include "initramfs.h"
#include "mm.h"

#define PM_PASSWORD 0x5a000000
#define PM_RSTC 0x3F10001c
#define PM_WDOG 0x3F100024

int shell_loop();

#endif