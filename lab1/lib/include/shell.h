#ifndef __SHELL_H__
#define __SHELL_H__
#include "mbox.h"
#include "stdint.h"
#include "string.h"
// #include "uart.h"

struct func {
    char *name;
    void (*ptr)();
    char *desc;
};

void welcome_msg();
void shell();
void read_cmd();
void exec_cmd();

void cmd_help();
void cmd_hello();
//void cmd_reboot();
void cmd_sysinfo();
void cmd_unknown();

struct func func_list[] = {
    {.name = "help", .ptr = cmd_help, .desc = "print this help menu"},
    {.name = "hello", .ptr = cmd_hello, .desc = "print Hello World!"},
    {.name = "reboot", .ptr = cmd_help, .desc = "reboot the device"},
    {.name = "sysinfo", .ptr = cmd_sysinfo, .desc = "get system info"},
    {.name = "123", .ptr = cmd_hello, .desc = "???"}};

#endif