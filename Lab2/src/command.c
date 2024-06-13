#include "../include/command.h"
#include "../include/uart.h"
#include "../include/cpio_parser.h"
#include "../include/shell.h"
#include "../include/my_stddef.h"
#include "../include/my_string.h"
#include "../include/my_stdlib.h"

void set(long addr, unsigned int value) {
    volatile unsigned int* point = (unsigned int*)addr;
    *point = value;
}

char *get_string(void){
    int buffer_counter = 0;
    char input_char;
    static char buffer[TMP_BUFFER_LEN];
    strset(buffer, 0, TMP_BUFFER_LEN);
    while (1) {
        input_char = uart_getc();
        if (!(input_char < 128 && input_char >= 0)) {
            uart_puts("Invalid character received\n");
            return NULL;
        } else if (input_char == LINE_FEED || input_char == CRRIAGE_RETURN) {
            uart_send(input_char);
            buffer[buffer_counter] = '\0';
            return buffer;
        }else {
            uart_send(input_char);

            if (buffer_counter < TMP_BUFFER_LEN) {
                buffer[buffer_counter] = input_char;
                buffer_counter++;
            }
            else {
                uart_puts("\nError: Input exceeded buffer length. Buffer reset.\n");
                buffer_counter = 0;
                strset(buffer, 0, TMP_BUFFER_LEN);

                // New line head
                uart_puts("# ");
            }
        }
    }
}

void cmd_help(void) {
    uart_puts("help        : print this help menu\n");
    uart_puts("hello       : print Hello World!\n");
    uart_puts("reboot      : reboot the device\n");
    uart_puts("ls          : show file in rootfs\n");
    uart_puts("cat         : show file content\n");
    uart_puts("malloc      : malloc demp\n");
}

void cmd_hello(void) {
    uart_puts("Hello World!\n");
}

void cmd_reboot(void) {
    uart_puts("Start rebooting...\n");
    set(PM_RSTC, PM_PASSWORD | 0x20);
}

void cmd_ls(void) {
    char *cpio_archive_start = (char *)RAPSPI_CPIO_ADDRESS;
    cpio_ls(cpio_archive_start);
}

void cmd_cat(void){
    uart_puts("Filename: ");
    char *filename = get_string();
    if (filename == NULL) {
        uart_puts("Error: Invalid filename\n");
        return;
    }
    char *cpio_archive_start = (char *)RAPSPI_CPIO_ADDRESS;
    cpio_cat(cpio_archive_start,filename);
}

void cmd_malloc(void) {
    char *ptr1 = simple_malloc(7);
    ptr1[0]='M';
    ptr1[1]='A';
    ptr1[2]='L';
    ptr1[3]='L';
    ptr1[4]='O';
    ptr1[5]='C';
    ptr1[6]='\0';
    uart_puts(ptr1);
    uart_puts("\n");
}


command_t commands[] = {
    {"help", "print this help menu", cmd_help},
    {"hello", "print Hello World!", cmd_hello},
    {"reboot", "reboot the device", cmd_reboot},
    {"ls", "show file in rootfs", cmd_ls},
    {"cat", "show file content", cmd_cat},
    {"malloc", "malloc demo", cmd_malloc},
    {NULL, NULL, NULL} // Sentinel value to mark the end of the array
};