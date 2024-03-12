#include "cmd.h"

#include "mailbox.h"
#include "mini-uart.h"

void cmd_help(const cmd_t*);
void cmd_hello(const cmd_t*);
void cmd_hwinfo(const cmd_t*);
void cmd_reboot(const cmd_t*);

const cmd_t cmds[] = {
    {
        .name = "help",
        .help = "print this help menu",
        .fp = cmd_help,
    },
    {
        .name = "hello",
        .help = "print Hello World!",
        .fp = cmd_hello,
    },
    {
        .name = "hwinfo",
        .help = "print hardware's infomation",
        .fp = cmd_hwinfo,
    },
    {
        .name = "reboot",
        .help = "reboot the device",
        .fp = cmd_reboot,
    },
};
const cmd_t cmds_end[0];

void cmd_help(const cmd_t* cmd) {
  for (const cmd_t* cmd = cmds; cmd != cmds_end; cmd++) {
    mini_uart_printf("%s\t%s\n", cmd->name, cmd->help);
  }
}

void cmd_hello(const cmd_t* cmd) {
  mini_uart_puts("Hello World!\n");
}

void cmd_hwinfo(const cmd_t* cmd) {
  mini_uart_printf("Board revision:\t0x%x\n", get_board_revision());
}

void cmd_reboot(const cmd_t* cmd) {
  // TODO
}
