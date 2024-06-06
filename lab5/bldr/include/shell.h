#ifndef SHELL_H
#define SHELL_H

#include "uart.h"
#include "string.h"
#include "load.h"

void shell();
void cmd_resolve(char *cmd);

#endif