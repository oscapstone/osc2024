#include "cmd.hpp"

#include "board/mini-uart.hpp"
#include "string.hpp"

int help_idx = 0;
const Cmd cmds[] = {
    {
        ._name = "help",
        ._help = "print this help menu",
        ._fp = cmd_help,
    },
    {
        ._name = "hello",
        ._help = "print Hello World!",
        ._fp = cmd_hello,
    },
    {
        ._name = "hwinfo",
        ._help = "print hardware's infomation",
        ._fp = cmd_hwinfo,
    },
    {
        ._name = "reboot",
        ._help = "reboot the device",
        ._fp = cmd_reboot,
    },
    {
        ._name = "ls",
        ._help = "list directory contents",
        ._fp = cmd_ls,
    },
};
constexpr int ncmd = sizeof(cmds) / sizeof(cmds[0]);

void runcmd(const char* buf, int len) {
  const Cmd* cmd = nullptr;
  for (int i = 0; i < ncmd; i++) {
    auto it = &cmds[i];
    if (!strcmp(buf, it->name())) {
      cmd = it;
      break;
    }
  }
  if (cmd != nullptr) {
    cmd->fp()();
  } else {
    mini_uart_printf("command not found: %s\n", buf);
  }
}
