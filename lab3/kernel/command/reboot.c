#include "kernel/bsp/reboot.h"

#include "all.h"
#include "kernel/io.h"

void _reboot_command(int argc, char **argv) {
    print_string("\nRebooting ...\n");
    reset(200);
}

struct Command reboot_command = {.name = "reboot",
                                 .description = "reboot the device",
                                 .function = &_reboot_command};
