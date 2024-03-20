#include "../include/shell.h"
#include "../include/uart1.h"

extern char* _bootloader_relocated_addr; // assigned in linker, 0x30000000
extern unsigned long long __code_size;
extern unsigned long long _start;
char* _dtb;

int relocated_flag = 1;

/* copy code block from _start to addr */
void code_relocate(char* addr)
{
	unsigned long long size = (unsigned long long)&__code_size;
	char* start = (char*)&_start;
	for (unsigned long long i = 0; i < size; i++) {
		addr[i] = start[i];
	}

	// after mov itself to 0x30000000, re-activate itself
	// it force addr to be a function ptr, 1 param type "char*" with return type "void", and pass "_dtb" into it
	((void (*)(char*))addr)(_dtb);
}

/* x0-x7 are argument registers.
   x0 is now used for dtb */
void main(char* arg)
{
	_dtb = arg;
	char* relocated_ptr = (char*)&_bootloader_relocated_addr;

	/* relocate once only */
	if (relocated_flag) {
		relocated_flag = 0;
		code_relocate(relocated_ptr);
	}

	char input_buffer[CMD_MAX_LEN];

	uart_init();
	cli_print_banner();

	while (1) {
		cli_cmd_clear(input_buffer, CMD_MAX_LEN);
		uart_puts("# ");
		cli_cmd_read(input_buffer);
		cli_cmd_exec(input_buffer);
	}
}