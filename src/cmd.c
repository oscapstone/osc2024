#include "cmd.h"

#include "mailbox.h"
#include "mini-uart.h"

void cmd_help(const Cmd_t*);
void cmd_hello(const Cmd_t*);
void cmd_hwinfo(const Cmd_t*);
void cmd_reboot(const Cmd_t*);

const Cmd_t cmds[] = {
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
const Cmd_t cmds_end[0];

void cmd_help(const Cmd_t* cmd) {
  for (const Cmd_t* cmd = cmds; cmd != cmds_end; cmd++) {
    mini_uart_printf("%s\t%s\n", cmd->name, cmd->help);
  }
}

void cmd_hello(const Cmd_t* cmd) {
  mini_uart_puts("Hello World!\n");
}

void cmd_hwinfo(const Cmd_t* cmd) {
  mini_uart_printf("Board revision:\t0x%x\n", get_board_revision());
}

void cmd_reboot(const Cmd_t* cmd) {
  // TODO
}
