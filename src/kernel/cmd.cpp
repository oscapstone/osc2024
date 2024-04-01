#include "cmd.hpp"

#include "board/mini-uart.hpp"
#include "reloc.hpp"
#include "string.hpp"

const Cmd cmds[] = {
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
    {
        .name = "ls",
        .help = "list directory contents",
        .fp = cmd_ls,
    },
    {
        .name = "cat",
        .help = "concatenate and print files",
        .fp = cmd_cat,
    },
    {
        .name = "alloc",
        .help = "memory allocation",
        .fp = cmd_alloc,
    },
    {
        .name = "devtree",
        .help = "print device tree",
        .fp = cmd_devtree,
    },
    {
        .name = "run",
        .help = "run user program",
        .fp = cmd_run,
    },
};
const int ncmd = sizeof(cmds) / sizeof(cmds[0]);

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
    if (!strcmp(argv[0], reloc(it->name))) {
      cmd = it;
      break;
    }
  }

  if (cmd != nullptr) {
    return reloc(cmd->fp)(argc, argv);
  } else {
    mini_uart_printf("command not found: %s\n", buf);
    return -1;
  }
}
