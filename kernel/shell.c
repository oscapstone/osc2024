#include "initramfs.h"
#include "mailbox.h"
#include "shell.h"
#include "string.h"
#include "uart.h"

void shell_start()
{
    uart_puts("# ");

    char line[MAX_GETLINE_LEN];
    char c;
    unsigned int index = 0;
    while (1) {
        c = (char) uart_getc();

        if (c == '\n') {
            line[index++] = '\0';
            uart_puts("\n");

            do_cmd(line);

            index = 0;
            uart_puts("# ");
        } else {
            line[index++] = c;
            uart_send(c);
        }
    }
}

// Define Shell Commands
static cmt_t command_funcs[] = {
    cmd_help,
    cmd_hello,
    cmd_reboot,
    get_board_revision,
    get_arm_memory,
    list_initramfs,
    cat_initramfs
};
static char* commands[] = {
    "help",
    "hello",
    "reboot",
    "board",
    "arm",
    "ls",
    "cat"
};
static char* command_descriptions[] = {
    "print this help menu",
    "print Hello World!",
    "reboot the device",
    "print board info",
    "print arm memory info",
    "list initramfs",
    "cat a file"
};

void do_cmd(const char* line)
{
    int size = sizeof(command_funcs) / sizeof(command_funcs[0]);
    for (int i = 0; i < size; i++) {
        if (strcmp(line, commands[i]) == 0) {
            command_funcs[i]();
            return;
        }
    }
    cmd_default();
    return;
}

void cmd_help()
{
    int size = sizeof(command_descriptions) / sizeof(command_descriptions[0]);
    for (int i = 0; i < size; i++) {
        uart_puts(commands[i]);
        uart_puts("\t: ");
        uart_puts(command_descriptions[i]);
        uart_puts("\n");
    }
}

void cmd_hello()
{
    uart_puts("Hello World!\n");
}

void cmd_reboot()
{
    uart_puts("Reboot!\n");

    unsigned int r;
    r = *PM_RSTS;
    r &= ~0xFFFFFAAA;
    *PM_RSTS = PM_WDOG_MAGIC | r;
    *PM_WDOG = PM_WDOG_MAGIC | 10;
    *PM_RSTC = PM_WDOG_MAGIC | PM_RSTC_FULLRST;
}

void cmd_default()
{
    uart_puts("command not found\n");
}