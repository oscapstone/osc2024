#ifndef __COMMAND_H__
#define __COMMAND_H__

#define PM_PASSWORD (unsigned int)0x5a000000
#define PM_RSTC     (volatile unsigned int*)0x3F10001c
#define PM_WDOG     (volatile unsigned int*)0x3F100024

#define MAX_COMM_NUM 6

typedef struct{
  const char *name;
  const char *description;
  int (*func)(void);
}Command;

int help();
int hello();
int reboot();
int lshw();
int ls();
int cat();

extern Command commands[];

#endif