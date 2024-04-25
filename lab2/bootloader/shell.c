#include"header/shell.h"
#include"header/uart.h"
#include"header/utils.h"
#include"header/reboot.h"
#include"header/bootloader.h"

void shell(char *dtb_base){
    char cmd[256];
    char *cur;
    while (1)
    {
        char *s = "# ";
        uart_send_str(s);
        cur = cmd;
        char receive;
        while (1)
        {
            receive = uart_get_char();
            if(receive == '\n'){
                *cur = '\0';
                break;
            }
            else if(receive == 127){
                if(cur == cmd){
                    *cur = '\0';
                    continue;
                }
                *cur = '\0';
                cur--;
                uart_send_str("\b \b");
                continue;
            }
            *cur = receive;
            uart_send_char(receive);
            cur++;
        }
        cur = cmd;
        if(string_compare(cur,"help")){
            uart_send_str("\nhelp\t\t:print this help menu\r\n");
            uart_send_str("hello\t\t:print Hello World!\r\n");
            uart_send_str("load\t\t:load kernel image through uart\r\n");
            uart_send_str("reboot\t\t:reboot the device\r\n");
        }
        else if(string_compare(cur,"hello")){
            uart_send_str("\nHello World!\n");
        }
        else if(string_compare(cur,"load")){
            uart_send_str("\nload kernel...\n");
            load_img(dtb_base);
        }
        else if (string_compare(cur,"reboot")) {
           uart_send_str("\nRebooting....\n");
           reset(1000);
        
        }
        else
            uart_send_str("\n");
    }
}