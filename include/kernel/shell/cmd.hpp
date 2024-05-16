#pragma once

int runcmd(const char*, int);

int cmd_help(int, char*[]);
int cmd_hello(int, char*[]);
int cmd_hwinfo(int, char*[]);
int cmd_reboot(int, char*[]);
int cmd_ls(int, char*[]);
int cmd_cat(int, char*[]);
int cmd_mm(int, char*[]);
int cmd_devtree(int, char*[]);
int cmd_exec(int, char*[]);
int cmd_run(int, char*[]);
int cmd_setTimeout(int, char*[]);
int cmd_uart(int, char*[]);
int cmd_demo(int, char*[]);
int cmd_schedule(int, char*[]);
int cmd_ps(int, char*[]);
int cmd_kill(int, char*[]);

using cmd_fp = int (*)(int, char*[]);

struct Cmd {
  const char* name;
  const char* help;
  cmd_fp fp;
};

extern const Cmd cmds[];
extern const int ncmd;
