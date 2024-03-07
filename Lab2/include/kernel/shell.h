#ifndef SHELL_H
#define SHELL_H

#include "kernel/uart.h"
#include "kernel/utils.h"
#include "kernel/mailbox.h"
#include "kernel/reboot.h"
#include "kernel/cpio.h"

#define MAX_BUF_LEN 256

void my_shell();

#endif