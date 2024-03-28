#include "shell.h"

void readcmd(char *x)
{
    char input_char;
    int input_index = 0;
    x[0] = 0;
    while( ((input_char = read_char()) != '\n'))
    {
        x[input_index] = input_char;
        ++input_index;
        printfc(input_char);
    }

    x[input_index]=0; // null char
}

void shell()
{
    printf("\nyuchang@raspberrypi3: ~$ ");
    char command[256];
    readcmd(command);

    if( (strcmp(command, "hello") == 0) )
    {
        printf("\nHello, World!");
    }
    else if( (strcmp(command, "help") == 0) )
    {
        printf("\nhelp\t: print this help menu");
        printf("\nhello\t: print Hello, World!");
        printf("\nmailbox\t: print mailbox info");
        printf("\nls\t: list files in the cpio archive");
        printf("\ncat\t: print file content in the cpio archive");
        printf("\nmalloc\t: test malloc function");
        printf("\nreboot\t: reboot the device");
    }
    else if ( (strcmp(command, "mailbox") == 0) )
    {
        printf("\nMailbox info:");
        get_board_revision();
        get_memory_info();
    }
    else if ( (strcmp(command, "ls") == 0) )
    {
        cpio_list();
    }
    else if( (strcmp(command, "cat") == 0) )
    {
        cpio_cat();
    }
    else if( (strcmp(command, "malloc") == 0) )
    {
        printf("\n1. char* p = simple_malloc(10);");
        char* p = simple_malloc(10);
        strcpy(p, "123456789");
        printf("\nMemory content: ");
        for(int i=0; i<10; i++){
            printfc(p[i]);
        }

        printf("\n2. char* p2 = simple_malloc(20);");
        char* p2 = simple_malloc(20);
        strcpy(p2, "1122334455667788990");
        printf("\nMemory content: ");
        for(int i=0; i<20; i++){
            printfc(p2[i]);
        }
    }
    else if( (strcmp(command, "reboot") == 0) )
    {
        printf("\nRebooting...\n");
        reset(200);
    }
    else
    {
        printf("\nCommand not found: ");
        printf(command);
    }
}