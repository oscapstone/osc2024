#include "shell.h"
#include "uart.h"
#include "reboot.h"
#include "cpio.h"
#include "dtb.h"
#include "memory.h"
#include "string.h"

extern char* _dtb_ptr;
extern char* cpio_addr;

void get_command(char *command_string){
    char element;
    char *input_ptr = command_string; 
    while(1) {
        element = uart_getc();
        if(element == '\n') {
            *input_ptr = '\0';
            uart_puts("\n");
            break;
        }
        *input_ptr++ = element;
        uart_send(element);
    }
}

void shell(){
    char command_string[256];
    uart_puts("# ");
    // get command
    get_command(command_string);
    // read command
    if(strcmp(command_string,"help")) {
        uart_puts("help	: print this help menu\n");
        uart_puts("ls	: list all files\n");
        uart_puts("cat	: print file content\n");
        uart_puts("malloc    : allocate memory\n");
        uart_puts("dtb	: print device tree\n");
        uart_puts("reboot	: print file content\n");
    } else if (strcmp(command_string,"ls")) {
        fdt_traverse(_dtb_ptr, initramfs_callback);
        cpio_list(cpio_addr);
    } else if (strcmp(command_string,"cat")) {
        fdt_traverse(_dtb_ptr, initramfs_callback);
        char filename[256];
        uart_puts("Enter filename: ");
        get_command(filename);
        cpio_cat(cpio_addr, filename);
    } else if (strcmp(command_string,"malloc")) {
        char* p = simple_malloc(10);
        uart_puts("\nCopying 123456789 to memory");
        strcpy(p, "123456789");
        uart_puts("\nMemory content: ");
        for(int i=0; i<10; i++){
            uart_send(p[i]);
        }
        uart_puts("\nCopying 987654321 to memory");
        strcpy(p, "987654321");
        uart_puts("\nMemory content: ");
        for(int i=0; i<10; i++){
            uart_send(p[i]);
        }
        uart_puts("\n");

    } else if (strcmp(command_string,"dtb")) {
        fdt_traverse(_dtb_ptr, show_tree_callback);
    } else if (strcmp(command_string,"reboot")) {
        uart_puts("Rebooting....\n");
        reset(1000);
    }
}