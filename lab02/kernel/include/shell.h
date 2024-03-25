#ifndef __SHELL_H__
#define __SHELL_H__

#include "mini_uart.h"
#include "mailbox.h"
#include "reboot.h"
#include "io.h"
#include "string.h"
#include "cpio.h"
#include "lib.h"

void readcmd(char *x);
void shell();

#endif