#include "shell.h"
#include "uart1.h"

extern char *_bootloader_relocated_addr;
extern unsigned long long __code_size;
extern unsigned long long _start;
char *_dtb;

int relocated_flag = 1;

/* Copies codeblock from _start to addr */
void code_relocate(char *addr)
{
    unsigned long long size = (unsigned long long)&__code_size;
    char *start = (char *)&_start;
    for (unsigned long long i = 0; i < size; i++) {
        addr[i] = start[i];
    }

    ((void (*)(char *))addr)(_dtb);
}

/* x0-x7 are argument registers.
   x0 is now used for dtb */
void main(char *arg)
{
    _dtb = arg;
    char *relocated_ptr = (char *)&_bootloader_relocated_addr;

    /* Relocate once only */
    if (relocated_flag) {
        relocated_flag = 0;
        code_relocate(relocated_ptr);
    }

    char input_buffer[CMD_MAX_LEN];

    uart_init();

    while (1) {
        cli_cmd_clear(input_buffer, CMD_MAX_LEN);
        uart_puts("# ");
        cli_cmd_read(input_buffer);
        cli_cmd_exec(input_buffer);
    }
}
