#include "shell.h"

void print_menu(){
    print_str("\nhelp       : print this help menu");
    print_str("\nhello      : print Hello World!");
    print_str("\nmailbox    : print mailbox info");
    print_str("\nreboot     : reboot the device");  
    print_str("\ncat        : cat all files in file system");
    print_str("\nls         : list all files in file system");
    print_str("\nmalloc     : dynamic allocate memory");
}

void print_rpi_info(){
    get_board_revision();
    get_memory_info();
}

void reboot(){
    reset(200);
}

void cat(){
    cpio_cat();
}

void ls(){
    cpio_ls();
}

void malloc(){
    print_str("\n----------");
    print_str("\nTesting allocating memory for \"short str\"");
    char* str1 = (char*)simple_alloc(10);
    strncpy("short str", str1, 10);

    print_str("\nstr1 = ");
    print_str(str1);

    print_str("\nAddress = 0x");
    print_hex((uint32_t)str1);

    print_str("\n----------");
    print_str("\nTesting allocating memory for \"Long Long String in RPI\"");
    char* str2 = (char*)simple_alloc(30);
    strncpy("Long Long String in RPI", str2, 30);

    print_str("\nstr2 = ");
    print_str(str2);

    print_str("\nAddress = 0x");
    print_hex((uint32_t)str2);
}

void shell(){
    print_str("\nAngus@Rpi3B+ > ");

    char cmd[256];

    read_input(cmd);

    if (strcmp(cmd, "help")){
        print_menu();
    }else if (strcmp(cmd, "hello")){
        print_str("\nHello World!");
    }else if (strcmp(cmd, "mailbox")){
        print_str("\nMailbox info :");
        print_rpi_info();
    }else if (strcmp(cmd, "reboot")){
        print_str("\nRebooting...");
        reboot();
    }else if (strcmp(cmd, "cat")){
        cat();   
    }else if (strcmp(cmd, "ls")){
        ls();
    }else if (strcmp(cmd, "malloc")){
        malloc();
    }else{
        print_str("\nCommand Not Found");
    }
}