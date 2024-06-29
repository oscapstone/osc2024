#pragma once

#define MAX_BUF_SIZE 1024
#define END_OF_COMMAND_LIST "NULL"

struct command {
  const char *name;
  const char *help;
  void (*func)(void);
};

extern struct command commands[];

// Commands
void cmd_help();
void cmd_hello();
void cmd_reboot();
void cmd_cancel();
void cmd_info();
void cmd_ls();
void cmd_cat();
void cmd_run();
void cmd_clear();
void cmd_timer();
void cmd_lab();