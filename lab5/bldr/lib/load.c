#include "load.h"

char *dtb_base;

void loadIMG()
{
    char *kernel = (char *)(0x80000);
    char buf[8];
    int buf_ptr = 0;
    int size;

    uart_printf("Please pass the kernel\n");

    while (1)
    {
        char c = uart_getcRAW();
        if (c == '\n')
        {
            buf[buf_ptr] = '\0';
            break;
        }
        else
            buf[buf_ptr++] = c;
    }

    size = atoi(buf);

    uart_printf("Kernel image size: %d\n", size);

    int kn_ptr = 0;
    while (size--)
    {
        kernel[kn_ptr++] = uart_getcRAW();
        if (kn_ptr % (1 << 10) == 0)
            uart_printf("Received %d bytes\r", kn_ptr);
    }
    uart_printf("Received %d bytes\n", kn_ptr);
    void (*run)(char *) = (void *)kernel;
    run(dtb_base);
}