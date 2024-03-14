#include "mailbox.h"
// #include "reboot.h"
#include "uart.h"
#include "io.h"

int strcmp(char* str1, char* str2){
    char* p1 = str1;
    char* p2 = str2;

    while (*p1 == *p2){
        if (*p1 == '\0')
            return 1;
        
        p1++;
        p2++;
    }

    return 0;
}

void shell(){
    print_str("\nAngus@Rpi3B+ > ");

    char cmd[256];

    readcmd(cmd);

    if (strcmp(cmd, "help")){
        print_str("\nhelp       : print this help menu");
        print_str("\nhello      : print Hello World!");
        print_str("\nreboot     : reboot the device");        
    }else if (strcmp(cmd, "hello")){
        print_str("\nHello World!");
    }else if (strcmp(cmd, "mailbox")){
        print_str("\nMailbox info :\n");
        get_board_revision();
        
    }else if (strcmp(cmd, "reboot")){
        print_str("\nRebooting...");
    }else{
        print_str("\nCommand Not Found");
    }
}

int main(){

    uart_init();
    print_str("\nLogin Shell");

    while(1){
        shell();
    }

    return 0;
}