#include "../bsp/reboot.h"

#include "../io.h"
#include "all.h"

void _reboot_command(int argc, char **argv) {
    print_string("\nRebooting ...\n");
    reset(200);
}

struct Command reboot_command = {.name = "reboot",
                                 .description = "reboot the device",
                                 .function = &_reboot_command};
