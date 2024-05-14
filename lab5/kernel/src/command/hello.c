#include <kernel/commands.h>
#include <kernel/io.h>

void _hello_command(int argc, char **argv) { print_string("\nHello World!"); }

struct Command hello_command = {.name = "hello",
                                .description = "print Hello World!",
                                .function = &_hello_command};
