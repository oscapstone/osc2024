#include "mini_uart.h"
#include "mbox.h"
#include "reboot.h"

unsigned int is_visible(unsigned int c){
    if(c >= 32 && c <= 126){
        return 1;
    }
    return 0;
}

unsigned int strcmp(char* str1, char* str2){
    int i = 0;
    while(str1[i] != '\0' && str2[i] != '\0'){
        if(str1[i] != str2[i]){
            return 0;
        }
        i++;
    }
    if(str1[i] != '\0' || str2[i] != '\0'){
        return 0;
    }
    return 1;
}


void uart_recv_command(char *str){
    char c;
    int i = 0;
    while(1){
        c = uart_recv();
        if(c == '\n'){
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

void shell(){
    uart_send_string("Welcome to OSC2024 shell!\n");
    while(1){
        uart_send_string("# ");
        char str[100];
        uart_recv_command(str);
        // uart_send_string(str);
        uart_send_string("\n");
        if(strcmp(str, "hello")){
            uart_send_string("Hello World!\n");
        } else if(strcmp(str, "mailbox")){
            uart_send_string("Mailbox info: \n");
            get_board_revision();
            get_memory_info();
        } else if (strcmp(str, "reboot")){
            uart_send_string("Rebooting...\n");
            reset(200);
            break;
        } else if(strcmp(str, "help")){
            uart_send_string("help\t: print this help menu\n");
            uart_send_string("hello\t: print Hello World!\n");
            uart_send_string("mailbox\t: print board revision and memory info\n");
            uart_send_string("reboot\t: reboot this device\n");
        }
    }
}