#ifndef _SHELL_H
#define _SHELL_H

#include "util.h"
#include "io.h"
#include "reboot.h"
#include "mailbox.h"
#include "cpio.h"
#include "timer.h"
#include "mini_uart.h"
#include "exception.h"
#include "mem.h"
#include "thread.h"
#include "syscall.h"

void print_menu();
void print_rpi_info();
void reboot();
void test_two_sec();
void test_timer();
void test_simple_alloc();
void test_preempt();
void test_malloc();

void shell();

#endif