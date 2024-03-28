#include "uart.h"
#include "utils.h"
#include "mailbox.h"
#include "reboot.h"
#include "cpio.h"
#include "mem.h"
#include "devtree.h"

#define MAX_LEN 1024

void *DEVICETREE_ADDRESS = 0;

uint64_t initrd_addr;

int readcmd(char *buf, int len);

void shell();

int main( )
{
    
    mini_uart_init();
    mem_init();

    print_string("\nWelcome to AnonymousELF's shell\n");
    // check_fdt_magic();

    uint32_t* t = 0x50000;
    print_char('\n');
    print_string("Dtb address 0x50000 contain is \n");
    print_h(*t);
    print_char('\n');

    asm volatile("mov %0, x20" :"=r"(DEVICETREE_ADDRESS));

    print_string("DEVICETREE_ADDRESS: ");
    print_h(DEVICETREE_ADDRESS);
    print_char('\n');
    fdt_traverse(initramfs_callback); //Use the API to get the address of initramfs instead of hardcoding it.

    while(1) {
        shell();
    }

    return 0;
}






void shell(){
    
    print_string("\nAnonymousELF@Rpi3B+ >>> ");

    char command[256];
    readcmd(command,256);
    static char buf[MAX_LEN];


    if(strcmp(command,"help")){
        print_string("\nhelp       : print this help menu\n");
        print_string("hello      : print Hello World!\n");
        print_string("mailbox    : print Hardware Information\n");
        print_string("ls         : list files existed in this folder\n");
        print_string("cat        : print specific file content\n");
        print_string("test malloc: test malloc function\n");
        print_string("reboot     : reboot the device");
    
    }else if(strcmp(command,"hello")){
        print_string("\nHello World!");
    
    }else if(strcmp(command,"mailbox")){
        print_string("\nMailbox info :\n");
        get_board_revision();
        get_memory_info();
    
    }else if(strcmp(command,"reboot")){
        print_string("\nRebooting ...\n");
        reset(200);

    }else if(strcmp(command,"ls")){
        cpio_ls();
    
    }else if(strcmp(command,"cat")){
        cpio_cat();

    }else if(strcmp(command,"test malloc")){
        char* test_str;
        test_str = "success";
        test_malloc(strlen(test_str), test_str);
        test_str = "test malloc successful";
        test_malloc(strlen(test_str), test_str);

    }
    else{
        print_string("\nCommand not found");
    
    }


}

void test_malloc(int num, char *str){
        print_string("\nTest str = malloc(");
        print_num(num);
        print_string(") ; str = ");
        print_string(str);
        print_string("\n");

        char *str_tmp = simple_malloc(num);
        strncpy(str_tmp,str,num);
        str_tmp[num]=0;
        print_string("Answer str = ");
        print_string(str_tmp);
        print_char('\n');
        print_string("Answer str address = ");
        print_h(str_tmp);
        print_char('\n');
}
