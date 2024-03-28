#ifndef __SHELL_H
#define __SHELL_H

#include "mini_uart.h"
#include "utils.h"


int help(int, char **);
int hello(int, char **);
int loadimg(int, char **);
int printimg(int, char **);
int write(int, char **);
int read(int, char **);
int shell_loop();

#endif