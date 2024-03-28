#ifndef _BOOT_SHELL_H_
#define _BOOT_SHELL_H_

#include "uart.h"

void read(char *buf, int len);
void bootloader_shell(char *dtb_base);

#endif