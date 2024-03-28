#include "io.h"

void printf(char* str)
{
    uart_send_string(str);
}

void printfc(char c)
{
    uart_send(c);
}

void printf_hex(unsigned int d)
{
    printf("0x");
    unsigned int td = d;
    for(int i=28; i>=0; i-=4){
        int tmp = (td >> i) & 0xf;
        if(tmp < 10){
            printfc('0'+tmp);
        }
        else{
            printfc('a'+tmp-10);
        }
    }
}

char read_char()
{
    char c = uart_recv();
    return c;
}
