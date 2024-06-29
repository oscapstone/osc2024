#pragma once

#define MAX_BUF_SIZE 1024
#define END_OF_COMMAND_LIST "NULL"

struct command {
  const char *name;
  const char *help;
  void (*func)(void);
};

extern struct command cmd_list[];
extern unsigned int BOARD_REVISION;
extern unsigned int BASE_MEMORY;
extern unsigned int NUM_PAGES;

// Commands
void cmd_info();
void cmd_hello();