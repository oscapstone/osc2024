#include "uart.h"
#include "io.h"
#include "mailbox.h"
#include "reboot.h"


void read_cmd(char* command) 
{
    char input_char;
    int idx = 0;
    command[0] = 0;
    while((input_char = read_char()) != '\n') {
        command[idx] = input_char;
        idx += 1;
        print_char(input_char);
    }
    command[idx] = 0;
}

int strcmp(const char* command, const char* b)
{
    while(*command && *b) {
        if(*command != *b) {
            return 0;
        }
        command++;
        b++;
    }
    return 1;
}

void shell()
{
    print_string("\nPiShell> ");

    char command[256];
    read_cmd(command);
    if(strcmp(command, "help")) 
    {
        print_string("\nhelp    : print this help menu ");
        print_string("\nhello   : print Hello World! ");
        print_string("\nreboot  : reboot the device ");
        print_string("\nmailbox : print mailbox info");
    }
    else if(strcmp(command, "hello")) 
    {
        print_string("\nHello World! ");
    }
    else if (strcmp(command, "reboot"))
    {
        print_string("\nRebooting... ");
        reset(200);
    }
    else if (strcmp(command, "mailbox"))
    {
        // get_board_revision();
        // get_memory_info();
        // get the board's unique serial number with a mailbox call
        mbox[0] = 8*4;                  // length of the message
        mbox[1] = MBOX_REQUEST;         // this is a request message
        
        mbox[2] = MBOX_TAG_GETMEMORY;   // get serial number command
        mbox[3] = 8;                    // buffer size
        mbox[4] = 0;
        mbox[5] = 0;                    // clear output buffer
        mbox[6] = 0;

        mbox[7] = MBOX_TAG_LAST;
        // send the message to the GPU and receive answer
        if (mbox_call(MBOX_CH_PROP)) {
            print_string("\nARM memory base address : ");
            print_h(mbox[5]);
            print_string("\r\n");

            print_string("ARM memory size : ");
            print_h(mbox[6]);
            print_string("\r\n");
        } else {
            uart_puts("\nUnable to query serial!\n");
        }
        //
        mbox[0] = 7*4;                  // length of the message
        mbox[1] = MBOX_REQUEST;         // this is a request message
        
        mbox[2] = MBOX_TAG_GET_REVISION;   // get serial number command
        mbox[3] = 4;                    // buffer size
        mbox[4] = 0;
        mbox[5] = 0;                    // clear output buffer
        mbox[6] = MBOX_TAG_LAST;
        // send the message to the GPU and receive answer
        if (mbox_call(MBOX_CH_PROP)) {
            print_string("board revision : ");
            print_h(mbox[5]);
            print_string("\r\n");
        } else {
            uart_puts("\nUnable to query serial!\n");
        }
    }
    else
    {
        print_string("\nUnknown command ");
    }
    
}

void main()
{
    // set up serial console
    uart_init();
    
    // say hello
    uart_puts("\nWelcome to Lab1 shell");
    
    // echo everything back
    // while(1) {
    //     uart_send(uart_getc());
    // }
    // shell
    while(1) {
        // print_string("\nHello World!");
        shell();
    }
}