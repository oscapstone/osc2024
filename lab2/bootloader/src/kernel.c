#include "mini_uart.h"
#include "shell.h"

extern char* _bootloader_relocated_addr;
extern unsigned long long __code_size;
extern unsigned long long _start;
char* _dtb;

int relocated_flag = 1;

/* Copy code block from _start to addr */
void code_relocate(char* addr) {
    unsigned long long size = (unsigned long long)&__code_size;
    char* start = (char *)&_start;
    for(unsigned long long i=0;i<size;i++)
    {
        addr[i] = start[i];
    }

    /* It is a function pointer to call the section '_start' again, 
       but this time with the dtb address as an argument saving in the register x0. */
    ((void (*)())addr)(_dtb);
}

/* x0-x7 are argument registers */
void kernel_main(char* arg) {
    /* x0 is now used for dtb */
    _dtb = arg;
    /* bootloader_relocated_add is defined in linker.ld */
	char* relocated_ptr = (char*)&_bootloader_relocated_addr;

    /* Relocate to relocated_ptr */
    if (relocated_flag) {
        relocated_flag = 0;
        code_relocate(relocated_ptr);
    }
	
	char input_buf[MAX_CMD_LEN];

	uart_init();
	cli_print_welcome_msg();

	while (1) {
		cli_clear_cmd(input_buf, MAX_CMD_LEN);
		uart_puts("LAB2(つ´ω`)つ@sh> ");
		cli_read_cmd(input_buf);
		cli_exec_cmd(input_buf);
	}
}
