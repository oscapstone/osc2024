#include "shell/cmd.hpp"

#include "io.hpp"
#include "reloc.hpp"
#include "shell/args.hpp"
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
        .name = "mm",
        .help = "memory allocation",
        .fp = cmd_mm,
    },
    {
        .name = "devtree",
        .help = "print device tree",
        .fp = cmd_devtree,
    },
    {
        .name = "exec",
        .help = "exec user program",
        .fp = cmd_exec,
    },
    {
        .name = "run",
        .help = "run user program",
        .fp = cmd_run,
    },
    {
        .name = "setTimeout",
        .help = "prints msg after secs",
        .fp = cmd_setTimeout,
    },
    {
        .name = "uart",
        .help = "toggle async uart",
        .fp = cmd_uart,
    },
    {
        .name = "demo",
        .help = "demo",
        .fp = cmd_demo,
    },
    {
        .name = "schedule",
        .help = "schedule",
        .fp = cmd_schedule,
    },
    {
        .name = "kill",
        .help = "kill",
        .fp = cmd_kill,
    },
    {
        .name = "ps",
        .help = "ps",
        .fp = cmd_ps,
    },
};
const int ncmd = sizeof(cmds) / sizeof(cmds[0]);

int runcmd(const char* buf, int len) {
  if (len <= 0)
    return 0;

  Args args(buf, len);

  const Cmd* cmd = nullptr;
  for (int i = 0; i < ncmd; i++) {
    auto it = &cmds[i];
    if (!strcmp(args[0], reloc(it->name))) {
      cmd = it;
      break;
    }
  }

  if (cmd != nullptr) {
    return reloc(cmd->fp)(args.size, args.array);
  } else {
    kprintf("command not found: %s\n", buf);
    return -1;
  }
}
