#include"header/shell.h"
#include"header/uart.h"
#include"header/utils.h"
#include"header/reboot.h"
#include"header/mailbox.h"
void shell(){
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
            uart_send_str("\nhelp	:print this help menu\n");
            uart_send_str("hello	:print Hello World!\n");
            uart_send_str("info	:Get the hardware's information\n");
            uart_send_str("reboot	:reboot the device\n");
        }
        else if(string_compare(cur,"hello")){
            uart_send_str("\nHello World!\n");
        }
        else if(string_compare(cur,"info")){
            uart_send_str("\nInfo:\n");
            uart_send_str("Board Vision: ");
            get_board_revision();
            get_memory_info();

        }
        else if (string_compare(cur,"reboot")) {
           uart_send_str("\nRebooting....\n");
           reset(1000);
        }  
        else
            uart_send_str("\n");
    }
    
}