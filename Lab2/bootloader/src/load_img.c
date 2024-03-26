#include "load_img.h"
#include "mini_uart.h"

int load_img()
{
    char* img_addr = (char*)KERNEL_LOAD_ADDR;
    uart_send_string("Please send the image size:\n");
    int img_size = 0;

    img_size = uart_recv_raw();
    img_size |= uart_recv_raw() << 8;
    img_size |= uart_recv_raw() << 16;
    img_size |= uart_recv_raw() << 24;

    uart_send_string("The image size is: ");
    uart_send_dec(img_size);
    uart_send_string("\n");

    if (img_size <= 0 || img_size >= 0x20000)
        return KERNEL_SIZE_ERROR;

    uart_send_string("Start to load the kernel image...\n");


    while (img_size--)
        *img_addr++ = uart_recv_raw();

    if ((unsigned long)img_addr >= 0x100000)
        return KERNEL_LOAD_ERROR;

    return KERNEL_LOAD_SUCCESS;
}
