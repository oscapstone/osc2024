#pragma once

#define PM_PASSWORD 0x5A000000
#define PM_RSTC (volatile unsigned int *)0x3F10001C
#define PM_WDOG (volatile unsigned int *)0x3F100024

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
void cmd_clear();
