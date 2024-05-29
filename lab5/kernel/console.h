#ifndef CONSOLE_H
#define CONSOLE_H

#include <kernel/commands.h>

struct Console;

struct Console *console_create();
void init_console(struct Console *console);
void run_console(struct Console *console);
void register_command(struct Console *console, command_t *command);
void register_all_commands(struct Console *console);
void destroy_console(struct Console *console);

#endif
