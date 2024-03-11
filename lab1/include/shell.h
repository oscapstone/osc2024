#ifndef _SHELL_H_
#define _SHELL_H_

#include "uart.h"

int strcmp(const char *s1, const char *s2);
void read(char *buf, int len);
void shell();

#endif