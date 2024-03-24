#include "load_img.h"
#include "mini_uart.h"
#include "utils.h"

void main(void)
{
    uart_init();
    uart_send_string("RBIN64\n");
    load_img();
}
