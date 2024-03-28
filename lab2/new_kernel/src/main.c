#include "mini_uart.h"
#include "shell.h"

extern char* __boot_loader_addr;
extern unsigned long long __code_size;
extern unsigned long long __begin;
char* _dtb;

int relocated_flag = 1;

// x0 is for the parameter
void main(char* arg)
{   
    // _dtb = arg;
    // char* relocated_ptr = (char*)&__boot_loader_addr;
    // uart_puts(&relocated_ptr);

    // // Once relocate, do not relocate again
    // if (relocated_flag)
    // {
    //     relocated_flag = 0;
    //     code_relocate(relocated_ptr);
    // }
    // char username[50];
    // char input_buffer[CMD_MAX_LEN];
    uart_init();
    // uart_puts("\nInput Your Username >> ");
    // shell_cmd_read(*username);
    shell_banner();
    
    while (1) {
        shell();
    }
}
void code_relocate(char* addr)
{
    unsigned long long size = (unsigned long long)&__code_size;
    char* start = (char *)&__begin;
    for(unsigned long long i=0;i<size;i++)
    {
        addr[i] = start[i];
    }

    ((void (*)(char*))addr)(_dtb);
}