#include "load_img.h"
#include "mini_uart.h"

void load_img(void)
{
    char* img_addr = (char*)0x80000;

    uart_send_string("Please send the image size:\n");
    int img_size = 0;

    img_size = uart_recv();
    img_size |= uart_recv() << 8;
    img_size |= uart_recv() << 16;
    img_size |= uart_recv() << 24;

    uart_send_string("The image size is: ");
    uart_send_dec(img_size);
    uart_send_string("\n");

    uart_send_string("Start to load the kernel image...\n");

    while (img_size--)
        *img_addr++ = uart_recv();

    uart_send_string("kernel loaded\n");

    asm volatile(
        "mov x30, 0x80000;"
        "ret");
}
