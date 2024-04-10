#ifndef _SHELL_H
#define _SHELL_H

#include "util.h"
#include "io.h"
#include "reboot.h"
#include "mailbox.h"
#include "cpio.h"
#include "memory.h"
#include "timer.h"
#include "mini_uart.h"
#include "exception.h"

void print_menu();
void print_rpi_info();
void reboot();
void test_two_sec();
void test_timer();
void test_malloc();
void test_preempt();

void shell();

#endif