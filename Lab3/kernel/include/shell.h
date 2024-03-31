#ifndef _SHELL_H
#define _SHELL_H

#include "util.h"
#include "io.h"
#include "reboot.h"
#include "mailbox.h"
#include "cpio.h"
#include "memory.h"

void print_menu();
void print_rpi_info();
void reboot();
void cat();
void ls();
void malloc();

void shell();

#endif