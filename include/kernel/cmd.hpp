#pragma once

int runcmd(const char*, int);

int cmd_help(int, char*[]);
int cmd_hello(int, char*[]);
int cmd_hwinfo(int, char*[]);
int cmd_reboot(int, char*[]);
int cmd_ls(int, char*[]);
int cmd_cat(int, char*[]);
int cmd_alloc(int, char*[]);

using cmd_fp = int (*)(int, char*[]);

struct Cmd {
  const char* name;
  const char* help;
  cmd_fp fp;
};

extern const Cmd cmds[];
extern const int ncmd;
