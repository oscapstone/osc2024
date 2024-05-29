#include <kernel/commands.h>
#include <kernel/bsp_port/reboot.h>
#include <kernel/io.h>

void _reboot_command(int argc, char **argv) {
    print_string("\nRebooting ...\n");
    reset(200);
}

command_t reboot_command = {.name = "reboot",
                                 .description = "reboot the device",
                                 .function = &_reboot_command};
