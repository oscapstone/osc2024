#include "c_utils.h"
#include "mini_uart.h"
#include "string.h"

void uart_recv_command(char *str){
    char c;
    int i = 0;
    while(1){
        c = uart_recv();
        if(c == '\r'){
            str[i] = '\0';
            break;
        } else if(c == 127 || c == 8){
            if(i > 0){
                i--;
                uart_send('\b');
                uart_send(' ');
                uart_send('\b');
            }
            continue;
        }
        if(is_visible(c)){
            str[i] = c;
            i++;
            uart_send(c);
        }
    }

}

int align4(int n)
{
	return n + (4 - n % 4) % 4;
}


int atoi(const char *s){
    int sign = 1;
    int i = 0;
    int result = 0;

    while(s[i] == ' ')
        i ++;
    
    if(s[i] == '-') {
        sign = -1;
        i++;
    }

    while(s[i] >= '0' && s[i] <= '9') {
        result = result * 10 + (s[i] - '0');
        i ++;
    }

    return sign * result;
}
