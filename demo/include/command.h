#ifndef COMMAND_H
#define COMMAND_H

#include "../include/uart.h"
#include "../include/cpio_parser.h"
#include "../include/shell.h"
#include "../include/my_stddef.h"
#include "../include/my_string.h"
#include "../include/my_stdlib.h"
#include "../include/timer.h"
#include "../include/task.h"
#include "../include/exception.h"
#include "../include/allocator.h"

#define PM_PASSWORD 0x5a000000
#define PM_RSTC 0x3F10001c
#define PM_WDOG 0x3F100024


typedef void (*command_handler_t)(int argc, char **argv);

typedef struct {
    const char *command;
    const char *description;
    command_handler_t handler;
} command_t;

extern command_t commands[];

void set(long addr, unsigned int value);
void cmd_help(int argc, char **argv);
void cmd_hello(int argc, char **argv);
void cmd_reboot(int argc, char **argv);
void cmd_ls(int argc, char **argv);
void cmd_cat(int argc, char **argv);
void cmd_malloc(int argc, char **argv);
void cmd_rup(int argc, char **argv);
void cmd_cctf(int argc, char **argv);
void cmd_async(int argc, char **argv);
void cmd_ttm(int argc, char **argv);
void cmd_sto(int argc, char **argv);
void cmd_tci(int argc, char **argv);
void cmd_atest(void);

#endif