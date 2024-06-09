#include "io.h"

void printf(const char* str)
{
    uart_send_string(str);
}

void printfc(const char c)
{
    uart_send(c);
}

void printf_hex(const unsigned int d)
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

void printf_int(const int d)
{
    int td = d;
    if(td < 0){
        printf("-");
        td = -td;
    }
    if(td == 0){
        printf("0");
        return;
    }
    int digits[10];
    int cnt = 0;
    while(td > 0){
        digits[cnt++] = td % 10;
        td /= 10;
    }
    for(int i=cnt-1; i>=0; i--){
        printfc('0'+digits[i]);
    }
}

char read_char()
{
    char c = uart_recv();
    return c;
}
