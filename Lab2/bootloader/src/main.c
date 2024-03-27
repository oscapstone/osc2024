#include "uart.h"
#include "bootloader.h"

void main()
{
    uart_init();
    load_img();
}