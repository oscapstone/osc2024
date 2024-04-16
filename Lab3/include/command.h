#ifndef __COMMAND_H__
#define __COMMAND_H__

#define PM_PASSWORD (unsigned int)0x5a000000
#define PM_RSTC     (volatile unsigned int*)0x3F10001c
#define PM_WDOG     (volatile unsigned int*)0x3F100024

#define MAX_COMM_NUM 9

typedef struct{
  const char *name;
  const char *description;
  int (*func)(void**);
}Command;

int help(void**);
int hello(void**);
int reboot(void**);
int lshw(void**);
int ls(void**);
int cat(void**);
int exec(void**);
int setTimeout(void**);
int test(void**);

extern Command commands[];

#endif