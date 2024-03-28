#include "uart1.h"
#include "shell.h"


#include "uart1.h"
#include "shell.h"


char* _dtb = 0;

int relocated_flag = 1;

/* Copies codeblock from _start to addr */
// void code_relocate(char* addr)
// {
//     unsigned long long size = (unsigned long long)&_bootloader_size;
//     char* start = (char *)&_start;
//     for(unsigned long long i=0;i<size;i++)
//     {
//         addr[i] = start[i];
//     }

//     ((void (*)(char*))addr)(_dtb);

// }

/* x0-x7 are argument registers.
   x0 is now used for dtb */
void main(char* arg){
    _dtb = arg;
    // char* relocated_ptr = (char*)&_bootloader_relocated_addr;
    // puts("_dtb:");
    // put_hex((unsigned int)_dtb);
    // puts("\r\n");
    // puts("_dtb_address in main: ");
    // put_hex((unsigned int)&_dtb);
    // puts("\r\n");

    // /* Relocate once only */
    // if (relocated_flag)
    // {
    //     relocated_flag = 0;
    //     code_relocate(relocated_ptr);
    // }

    uart_init();
    start_shell(_dtb);
}

