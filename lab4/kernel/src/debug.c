#include "debug.h"
#include "stdio.h"

void print_log_level()
{
#if _DEBUG >= 0 && _DEBUG <= 3
#ifdef QEMU
	puts("┌───────────────┐\r\n");
	puts("│ Logging level │\r\n");
	puts("├───────────────┤\n");
	ERROR_BLOCK({
		puts("│ ");
		ERROR("      │\r\n");
	});
	WARNING_BLOCK({
		puts("│ ");
		WARNING("    │\r\n");
	});
	INFO_BLOCK({
		puts("│ ");
		INFO("       │\r\n");
	});
	DEBUG_BLOCK({
		puts("│ ");
		DEBUG("      │\r\n");
	});
	puts("└───────────────┘\r\n\r\n");
#elif RPI
	puts("|===============|\r\n");
	puts("| Logging level |\r\n");
	puts("|---------------|\n");
	ERROR_BLOCK({
		puts("| ");
		ERROR("      |\r\n");
	});
	WARNING_BLOCK({
		puts("| ");
		WARNING("    |\r\n");
	});
	INFO_BLOCK({
		puts("| ");
		INFO("       |\r\n");
	});
	DEBUG_BLOCK({
		puts("| ");
		DEBUG("      |\r\n");
	});
	puts("|---------------|\r\n\r\n");
#endif
#endif
}