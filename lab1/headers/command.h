#ifndef __COMMAND_H
#define __COMMAND_H

#define COMMAND_HELP        "help"
#define COMMAND_HELLO       "hello"
#define COMMAND_INFO        "info"
#define COMMAND_REBOOT      "reboot"

void help(void);
void hello(void);
void info(void);
void reboot(void);
void undefined(void);

#endif