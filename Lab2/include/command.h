#ifndef COMMAND_H
#define COMMAND_H

#include "../include/my_stddef.h"

#define PM_PASSWORD 0x5a000000
#define PM_RSTC 0x3F10001c
#define PM_WDOG 0x3F100024

#define TMP_BUFFER_LEN 32

typedef void (*command_handler_t)(void);

typedef struct {
    const char *command;
    const char *description;
    command_handler_t handler;
} command_t;

extern command_t commands[];

void set(long addr, unsigned int value);
char *get_string(void);
void cmd_help(void);
void cmd_hello(void);
void cmd_reboot(void);
void cmd_ls(void);
void cmd_cat(void);
void cmd_malloc(void);

#endif