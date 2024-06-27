#ifndef CONSOLE_H
#define CONSOLE_H

#include <kernel/commands.h>


void init_console();
void run_console();
void register_command(command_t *command);
void register_all_commands();
void destroy_console();

#endif
