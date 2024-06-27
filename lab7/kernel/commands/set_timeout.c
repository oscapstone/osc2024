#include <kernel/commands.h>
#include <kernel/io.h>
#include <kernel/timer.h>

void print_timeout(int delay) {
    puts("\nTimeout: ");
    print_d((const int)delay);
    puts("s\n");
}

void _set_timeout(int argc, char **argv) {
    if (argc < 2) {
        puts("\nUsage: set_timeout <timeout>\n");
        return;
    }

    puts("\nSetting timeout: ");
    print_d(atoi(argv[1]));
    puts("s\n");

    unsigned long timeout = atoi(argv[1]);
    set_timeout((void *)print_timeout, (void *)timeout, atoi(argv[1]));
}

command_t set_timeout_command = {.name = "set_timeout",
                                      .description = "set a timeout",
                                      .function = &_set_timeout};
