#include "uart1.h"
#include "shell.h"

extern char* _bootloader_relocated_addr;
extern unsigned long long __code_size;  // External variable holding the size of the code section
extern unsigned long long _start;  // External variable pointing to the start of the code section
char* _dtb;

int relocated_flag = 1;

/* Copies codeblock from _start to addr */
void code_relocate(char* addr)
{
    unsigned long long size = (unsigned long long)&__code_size;
    char* start = (char *)&_start;
    for(unsigned long long i=0;i<size;i++)
    {
        addr[i] = start[i];  // Copy each byte from the start to the new address
    }

    ((void (*)(char*))addr)(_dtb);  // Jump to the relocated code, passing the DTB pointer as an argument
}

/* x0-x7 are argument registers.
   x0 is now used for dtb */
void main(char* arg){
    _dtb = arg; // Store the DTB pointer passed by the bootloader in x0
    char* relocated_ptr = (char*)&_bootloader_relocated_addr;

    /* Relocate once only */
    if (relocated_flag)
    {
        relocated_flag = 0;
        code_relocate(relocated_ptr);
    }

    char input_buffer[CMD_MAX_LEN];
    
    uart_init();
    uart_flush();
    
    cli_print_banner();

    while(1){
        cli_cmd_clear(input_buffer, CMD_MAX_LEN);
        uart_puts("# ");
        cli_cmd_read(input_buffer);
        cli_cmd_exec(input_buffer);
    }
}
