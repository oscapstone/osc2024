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
    {
        ._name = "cat",
        ._help = "concatenate and print files",
        ._fp = cmd_cat,
    },
    {
        ._name = "alloc",
        ._help = "memory allocation",
        ._fp = cmd_alloc,
    },
};
constexpr int ncmd = sizeof(cmds) / sizeof(cmds[0]);

int runcmd(const char* buf, int len) {
  if (len <= 0)
    return 0;

  char buf_[len + 1];
  memcpy(buf_, buf, len);
  buf_[len] = 0;

  int argc = 1;
  for (int i = 0; i < len; i++)
    if (buf_[i] == ' ') {
      buf_[i] = '\0';
      argc++;
    }
  char* argv[argc];
  argv[0] = buf_;
  for (int i = 1; i < argc; i++)
    argv[i] = argv[i - 1] + strlen(argv[i - 1]) + 1;

  const Cmd* cmd = nullptr;
  for (int i = 0; i < ncmd; i++) {
    auto it = &cmds[i];
    if (!strcmp(argv[0], it->name())) {
      cmd = it;
      break;
    }
  }

  if (cmd != nullptr) {
    return cmd->fp()(argc, argv);
  } else {
    mini_uart_printf("command not found: %s\n", buf);
    return -1;
  }
}
