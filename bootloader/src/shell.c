#include "shell.h"
#include "my_string.h"
#include "uart.h"


void shell_init()
{
    uart_init();
    uart_puts("\n\n Hello from Raspi 3b\n");
}

void shell_input(char *cmd)
{
    char c;
    int idx = 0, end = 0;

    while ((c = uart_getc()) != '\n') {
        uart_send(c);
        cmd[idx++] = c;
        cmd[++end] = '\0';
    }
}

void shell_controller(char *cmd)
{
    uart_send('\n');
    
    if (!strcmp(cmd, ""))
        return;
    else if (!strcmp(cmd, "help")) {
        uart_puts("help      : print this help menu\n");
        uart_puts("loadimg   : load image\n");
    } else if (!strcmp(cmd, "loadimg")) {
        load_img();
    } else {
        uart_puts("shell: command not found\n");
    }
}

void load_img()
{
    char c;
    char *kernel_address = (char *) 0x80000;

    int img_size = 0;
    for (int i = 0; i < 4; i++) {
        img_size <<= 8;
        img_size |= (int) uart_getb();
    }

    uart_puts("img_size: ");
    uart_hex(img_size);
    uart_send('\n');

    // Start receive image
    for (int i = 0; i < img_size; i++) {
        c = uart_getb();
        *(kernel_address + i) = c;
    }

    uart_puts("Jump to image\n");

    void (*start_os)(void) = (void *)kernel_address;
    start_os();
}