#include "uart.h"
#include "io.h"
#include "reboot.h"
#include "mailbox.h"

void cmd_read(char *buf){
    char tmp;
    int idx = 0;
    buf[0] = 0;
    while ((tmp = read_c()) != '\n')
    {
        if (tmp < 32 || tmp > 126)
            continue;
        buf[idx++] = tmp;
        print_char(tmp);
    }
    buf[idx] = 0;
}

int strcmp(char *a, char *b){
    while (*a && *b && *a == *b){
        a++;
        b++;
    }
    if (*a == '\0' && *b == '\0')
        return 1;
    return 0;
}

void shell(){
    while (1)
    {
        print_string(">>>> ");
        char buf[100];
        cmd_read(buf);
        print_string("\n");
        if (strcmp(buf, "hello")){
            print_string("Hello world!\n");
        } else if (strcmp(buf, "reboot")){
            print_string("Rebooting...\n");
            reset(200);
        }
        else if (strcmp(buf, "help")){
            print_string("help: print this message\nhello: print Hello, World!\nreboot: reboot the device\nmailbox: get board revision\n");
        }else if (strcmp(buf, "mailbox")){
            print_string("Mailbox: \n");
            get_board_revision();
            p_mem_info();
        }else{
            print_string("Unknown command\n");
        }
    }
}


int main(){
    uart_init();
    uart_puts("\nWelcome to ypwang's OS\n");
    shell();

    return 0;
}