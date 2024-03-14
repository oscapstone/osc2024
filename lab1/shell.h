#ifndef _SHELL_H
#define _SHELL_H

#define PM_PASSWORD 0x5a000000
#define PM_RSTC 0x3F10001c
#define PM_WDOG 0x3F100024

/* shell */
void shell();
void read_inst();
void compare_inst(char* buffer);

/* string */
int str_cmp(char* str1, char* str2);
void newline2end(char *str);
void uint_to_hex(unsigned int num);

/* reboot */
void set(long addr, unsigned int value);
void reset(int tick);
void cancel_reset();

#endif