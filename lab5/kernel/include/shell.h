#ifndef SHELL_H
#define SHELL_H

#include "malloc.h"
#include "cpio.h"
#include "timer.h"
#include "irq.h"

void cmd(char *s1);
void shell();

#endif