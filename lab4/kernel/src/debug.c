#include "debug.h"
#include "uart1.h"

void print_log_level()
{
#if _DEBUG >= 0 && _DEBUG <= 3
#ifdef QEMU
	puts("┌───────────────┐\r\n");
	puts("│ Logging level │\r\n");
	puts("├───────────────┤\n");
	ERROR_BLOCK({
		uart_puts("│ ");
		ERROR("      │\r\n");
	});
	WARNING_BLOCK({
		uart_puts("│ ");
		WARNING("    │\r\n");
	});
	INFO_BLOCK({
		uart_puts("│ ");
		INFO("       │\r\n");
	});
	DEBUG_BLOCK({
		uart_puts("│ ");
		DEBUG("      │\r\n");
	});
	puts("└───────────────┘\r\n\r\n");
#elif RPI
	puts("|===============|\r\n");
	puts("| Logging level |\r\n");
	puts("|---------------|\n");
	ERROR_BLOCK({
		uart_puts("| ");
		ERROR("      |\r\n");
	});
	WARNING_BLOCK({
		uart_puts("| ");
		WARNING("    |\r\n");
	});
	INFO_BLOCK({
		uart_puts("| ");
		INFO("       |\r\n");
	});
	DEBUG_BLOCK({
		uart_puts("| ");
		DEBUG("      |\r\n");
	});
	puts("|---------------|\r\n\r\n");
#endif
#endif
}