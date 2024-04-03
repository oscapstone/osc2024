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

void printf_int(int d)
{
    if(d < 0){
        printf("-");
        d = -d;
    }
    if(d == 0){
        printf("0");
        return;
    }
    int td = d;
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
