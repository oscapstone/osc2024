#ifndef __COMMAND_H__
#define __COMMAND_H__

#define PM_PASSWORD (unsigned int)0x5a000000
#define PM_RSTC     (volatile unsigned int*)0x3F10001c
#define PM_WDOG     (volatile unsigned int*)0x3F100024

#define MAX_COMM_NUM 4

struct Command
{
  // enum Type type;
  const char *name;
  const char *description;
  int (*func)(void);
};

int help();
int hello();
int reboot();
int lshw();
enum Type inputToType(const char *command);

extern struct Command commands[];

#endif