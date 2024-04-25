#include"header/shell.h"
#include"header/uart.h"
#include"header/utils.h"
#include"header/reboot.h"
#include"header/mailbox.h"
#include"header/cpio.h"
#include"header/malloc.h"
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
        char arg[20][20];
        char *tk = strtok(cmd," ");
        for(int i = 0; tk != 0;i++){
            strcpy(arg[i],tk);
            tk = strtok(0," ");
        }
        if(string_compare(arg[0],"help")){
            uart_send_str("\nhelp\t\t:print this help menu\r\n");
            uart_send_str("hello\t\t:print Hello World!\r\n");
            uart_send_str("info\t\t:Get the hardware's information\r\n");
            uart_send_str("ls\t\t:list files in directory\r\n");
            uart_send_str("cat\t\t:cat\r\n");
            uart_send_str("clear\t\t:clear terminal\r\n");
            uart_send_str("reboot\t\t:reboot the device\r\n");
            uart_send_str("malloc\t\t:alloc string\r\n");
        }
        else if(string_compare(arg[0],"hello")){
            uart_send_str("\nHello World!\n");
        }
        else if(string_compare(arg[0],"info")){
            uart_send_str("\nInfo:\n");
            uart_send_str("Board Vision: ");
            get_board_revision();
            get_memory_info();

        }
        else if(string_compare(arg[0],"clear")){
            uart_send_str("\x1b[2J\x1b[H");
        }
        else if(string_compare(arg[0],"ls")){
            uart_send_str("\n");
            ls(".");
        }
        else if(string_compare(arg[0],"cat")){
            uart_send_str("\n");
            cat(arg[1]);
        }
        else if (string_compare(arg[0],"reboot")) {
           uart_send_str("\nRebooting....\n");
           reset(1000);
        }  
        else if (string_compare(arg[0],"malloc")){
            uart_send_str("\n");
            unsigned int size = (strlen(arg[1]) + 31) >> 5 << 5;
            char *string = simple_malloc(size);
            strcpy(string,arg[1]);
            uart_send_str(string);
            uart_send_str("\n");
        }
        else
            uart_send_str("\n");
    }
    
}