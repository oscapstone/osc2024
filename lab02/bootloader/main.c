#include "uart.h"
#include "utils.h"



#define BOOTLOADER_ADDRESS 0x60000
#define KERNEL_ADDRESS 0x80000
// #define DTB_ADDRESS 0x50000
extern uint32_t __bss_end;
extern uint32_t __bss_size;


uint64_t* Bootloader_head = (uint64_t*)KERNEL_ADDRESS;
uint64_t* Copy_head = (uint64_t*)BOOTLOADER_ADDRESS;
uint64_t* bss_end =(uint64_t*)(&__bss_end);

void *DEVICETREE_ADDRESS = 0;


void relocate();

void main(char* arg)
{   


    mini_uart_init();

    

    relocate();


    unsigned int Kernel_size = 0;
    print_string("Bootloater Start \n");
    asm volatile("b -131068"); // jump to same place in bootloader
    //0x20000 - 4

    print_string("Type in Kernel Size\n");

    char tmp = mini_uart_recv();
    

    Kernel_size = tmp;
    print_char(tmp);
    

    
    int three = 3;
    while (three > 0){
        Kernel_size = Kernel_size << 8;
        tmp = mini_uart_recv();
        Kernel_size |= tmp;
        three--;
    }
    print_char('\n');



    print_string("Kernel Size is ");
    print_h(Kernel_size);
    print_char('\n');

    unsigned char *Kernel  = (unsigned char *)KERNEL_ADDRESS;
    unsigned char *Kernel_pointer = Kernel;
    print_string("Loading kernel...\n");
    for(int i = 0 ;i < Kernel_size ; i++){
        *Kernel_pointer = mini_uart_recv();
        Kernel_pointer ++;
        // print_num(i);
    }
    // print_string("Kernel pointer to ");
    // print_h((uint32_t)(Kernel_pointer-1));
    // print_char('\n');

    // print_string("Kernel Address is ");
    // print_h((uint32_t)KERNEL_ADDRESS);
    // print_char('\n');
    
    // print_string("Kernel pointer to ");
    // print_h((uint32_t)(Kernel_pointer-1));
    // print_char('\n');

    print_string("Kernel is loaded from ");
    print_h((uint32_t)Kernel);
    print_string(" to ");
    print_h((uint32_t)(Kernel_pointer-1));
    print_char('\n');
    
    // asm volatile("mov %0, x20" :"=r"(DEVICETREE_ADDRESS));

    // print_string("DEVICETREE_ADDRESS: ");
    // print_h(DEVICETREE_ADDRESS);
    // print_char('\n');

    // branch to kernel
    asm volatile("blr %0"
               :
               : "r" (0x80000));

    
}

void relocate(){



    print_string("Bootloader Address is :");
    print_h((uint32_t)Bootloader_head);
    print_char('\n');

    

    print_string("Copy Address is :");
    print_h((uint32_t)Copy_head);
    print_char('\n');

    print_string("bss end ");
    print_h((uint32_t)bss_end);
    print_char('\n');




    uint32_t i = 0;

    // bootloader relocate
    while(Bootloader_head != bss_end){
        *Copy_head = *Bootloader_head;
        
        Copy_head ++;
        Bootloader_head ++;
        i++;
    }
    print_string("bss length :");
    print_num(i);
    print_char('\n');



    print_string("Copy Done\n");
}


