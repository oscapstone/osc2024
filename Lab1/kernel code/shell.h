#ifndef MYSHELL_H
#define MYSHELL_H

#define MAX_BUFFER_LEN 64
#define NEW_LINE 1001
#define LINE_FEED 10
#define CRRIAGE_RETURN 13

#define PM_PASSWORD 0x5a000000
#define PM_RSTC 0x3F10001c
#define PM_WDOG 0x3F100024

void set();
void shell_start();
void command_controller (char c, char [], int *);

#endif
