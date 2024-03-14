#ifndef _ALL_H
#define _ALL_H

struct Command {
  char name[16];
  char description[64];
  void (*function)(int argc, char **argv);
};

extern struct Command hello_command;
extern struct Command mailbox_command;
extern struct Command reboot_command;

#endif
