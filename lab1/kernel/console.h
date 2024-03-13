#ifndef CONSOLE_H
#define CONSOLE_H

struct Command;
struct Console;

struct Console *console_create();
void init_console(struct Console *console);
void run_console(struct Console *console);
void register_command(struct Console *console, struct Command *command);
void destroy_console(struct Console *console);

#endif
