#include "uart.h"

void readfile(char *target, unsigned long file_size)
{
    while (file_size-- > 0)
        uart_send(*(target++));
}