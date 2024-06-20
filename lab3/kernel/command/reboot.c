#include "bsp/reboot.h"

#include "command.h"
#include "kernel/io.h"

void _reboot_command(int argc, char **argv)
{
	print_string("\nRebooting ...\n");
	reset(200);
}

struct command reboot_command = { .name = "reboot",
				  .description = "reboot the device",
				  .function = &_reboot_command };
