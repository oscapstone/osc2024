// main.c
#include "miniuart.h"
#include "mailbox.h"
#include "reboot.h"

int strcmp(const char *a, const char *b) {
    while (*a && *b && (*a == *b)) {
        ++a;
        ++b;
    }
    return *a - *b;
}

void readcmd(char *x){
    

    char input_char;
    x[0]=0;
    int input_index = 0;
    // uart_puts("readcmd start\n");
    while( (input_char = miniUARTRead()) != '\n'){
        x[input_index] = input_char ;
        input_index += 1;
        miniUARTWrite(input_char);
    }

    // miniUARTWrite('\n');
    x[input_index]=0;

}



void shell(){
    
    miniUARTWriteString("\nusername@Rpi3B+ >>> ");

    char command[256];
    readcmd(command);
    

    if(strcmp(command,"help")==0){
        miniUARTWriteString("\nhelp       : print this help menu\n");
        miniUARTWriteString("hello      : print Hello World!\n");
        miniUARTWriteString("reboot      : reboot the device\n");
    }
    else if(strcmp(command,"hello")==0){
        miniUARTWriteString("\nHello World!");
    }else if(strcmp(command,"mailbox")==0){
        miniUARTWriteString("\nMailbox info :\n");
        get_board_revision();
        get_memory_info();
    }
    else if(strcmp(command,"reboot")==0){
        miniUARTWriteString("\nRebooting ...\n");
        reset(200);

    }else{
        miniUARTWriteString("\nCommand not found");
    }


}


int main()
{
    // set up serial console
    miniUARTInit();
    miniUARTWriteString("\nWelcome to AnonymousELF's shell");
    while(1) {
        shell();
    }

    return 0;
}




