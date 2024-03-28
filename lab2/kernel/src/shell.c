#include "shell.h"
#include "uart.h"
#include "reboot.h"
#include "cpio.h"
#include "dtb.h"
#include "memory.h"

char* strcpy(char* dest, const char* src)
{
    char* ret = dest;
    while(*src){
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0';
    return ret;
}

int strcmp(const char* str1, const char* str2) {
    while (*str1 != '\n' || *str2 != '\0') {
        if (*str1 != *str2) {
            return 0;
        }
        str1++;
        str2++;
    }
    if (*str1 == '\n' && *str2 =='\0') {
        return 1;
    } else {
        return 0;
    }
}

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

extern char* _dtb_ptr;
extern char* cpio_addr;


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
        traverse_device_tree(_dtb_ptr, dtb_callback_initramfs);
        cpio_list(cpio_addr);
    } else if (strcmp(command_string,"cat")) {
        traverse_device_tree(_dtb_ptr, dtb_callback_initramfs);
        char filename[256];
        uart_puts("Enter filename: ");
        get_command(filename);
        cpio_cat(cpio_addr, filename);
    } else if (strcmp(command_string,"malloc")) {
        char* p = simple_malloc(10);
        uart_puts("Copying 123456789 to memory\n");
        strcpy(p, "123456789");
        uart_puts("\nMemory content: ");
        for(int i=0; i<10; i++){
            uart_send(p[i]);
        }
        uart_puts("Copying 987654321 to memory\n");
        strcpy(p, "987654321");
        uart_puts("\nMemory content: ");
        for(int i=0; i<10; i++){
            uart_send(p[i]);
        }
        uart_puts("\n");

    } else if (strcmp(command_string,"dtb")) {
        traverse_device_tree(_dtb_ptr, dtb_callback_show_tree);
    } else if (strcmp(command_string,"reboot")) {
        uart_puts("Rebooting....\n");
        reset(1000);
    }
}