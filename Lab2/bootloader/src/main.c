#include "load_img.h"
#include "logo.h"
#include "mini_uart.h"
#include "shell.h"
#include "utils.h"

unsigned long dtb_ptr;

void main(unsigned long x0)
{
    uart_init();
    uart_send_string("RBIN64\n");
    send_logo();
    // uart_send_string("DTB addr: 0x");
    // uart_send_hex(x0);
    // uart_send_string("\n");

    dtb_ptr = x0;

    shell();

    while (1)
        ;
}
