#ifndef __SHELL_H
#define __SHELL_H

#include "mini_uart.h"
#include "utils.h"


int help();
int hello();
int reboot();
int board_info();
int shell_loop();

#endif