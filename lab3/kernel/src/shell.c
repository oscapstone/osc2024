#include "shell.h"
#include "uart.h"
#include "reboot.h"
#include "cpio.h"
#include "dtb.h"
#include "memory.h"
#include "string.h"
#include "exception.h"
#include "timer.h"

extern char* _dtb_ptr;
extern char* cpio_addr;

int atoi(const char *str) {
    int res = 0;
    for (int i = 0; str[i] != '\0'; ++i) {
        res = res * 10 + str[i] - '0';
    }
    return res;
}

void get_command(char *command_string){
    char element;
    char *input_ptr = command_string; 
    while(1) {
        element = uart_getc();
        if(element == '\n') {
            *input_ptr = '\0';
            uart_puts("\n");
            break;
        }
        *input_ptr++ = element;
        uart_send(element);
    }
}

void shell(){
    char command_string[256];
    uart_puts("# ");
    get_command(command_string);
    char* parameters[5];
    int para_idx = 0;
    int command_len = strlen(command_string);
    for(int i = 0; i < command_len; i++){
        if(command_string[i] == ' '){
            command_string[i] = '\0';
            parameters[para_idx++] = &command_string[i] + 1;
        }
    }
    // read command
    if (strcmp(command_string,"help")) {
        uart_puts("help	: print this help menu\n");
        uart_puts("ls	: list all files\n");
        uart_puts("cat	: print file content\n");
        uart_puts("malloc    : allocate memory\n");
        uart_puts("dtb	: print device tree\n");
        uart_puts("exec	: exec the user program\n");
        uart_puts("timer	: Enable the timer’s interrupt\n");
        uart_puts("async	: enable async uart\n");
        uart_puts("setTimeout	: print MESSAGE after SECONDS\n");
        uart_puts("reboot	: print file content\n");
    } else if (strcmp(command_string,"ls")) {
        fdt_traverse(_dtb_ptr, initramfs_callback);
        cpio_list(cpio_addr);
    } else if (strcmp(command_string,"cat")) {
        fdt_traverse(_dtb_ptr, initramfs_callback);
        // char filename[256];
        // uart_puts("Enter filename: ");
        // get_command(filename);
        cpio_cat(cpio_addr, parameters[0]);
    } else if (strcmp(command_string,"malloc")) {
        char* p = simple_malloc(10);
        uart_puts("\nCopying 123456789 to memory");
        strcpy(p, "123456789");
        uart_puts("\nMemory content: ");
        for(int i=0; i<10; i++){
            uart_send(p[i]);
        }
        uart_puts("\nCopying 987654321 to memory");
        strcpy(p, "987654321");
        uart_puts("\nMemory content: ");
        for(int i=0; i<10; i++){
            uart_send(p[i]);
        }
        uart_puts("\n");
    } else if (strcmp(command_string,"dtb")) {
        fdt_traverse(_dtb_ptr, show_tree_callback);
    } else if (strcmp(command_string,"reboot")) {
        uart_puts("Rebooting....\n");
        reset(1000);
    } else if (strcmp(command_string,"exec")) {
        fdt_traverse(_dtb_ptr, initramfs_callback);
        // char filename[256];
        // uart_puts("Enter filename: ");
        // get_command(filename);

        cpio_exec(cpio_addr, parameters[0]);
    } else if (strcmp(command_string,"timer")) {
        disable_interrupt();

        core_timer_enable();
        set_core_timer(2 * get_core_frequency());

        void (*location)(void) = infinite;

        asm volatile( "msr     elr_el1, %0\r\n\t" ::"r" (location) );
        asm volatile( "mov     x0, 0x340\r\n\t");
        asm volatile( "msr     spsr_el1, x0\r\n\t");
        asm volatile( "mov     x0, sp\r\n\t");
        asm volatile( "msr     sp_el0, x0\r\n\t");
        asm volatile("eret    \r\n\t");
    } else if (strcmp(command_string,"async")){
        enable_uart_interrupt();
        
        char input[256];
        async_uart_puts("(async) # ");
        uint32_t input_len = async_uart_gets(input,256);
        if(input_len==0){
            async_uart_puts("\r\n");
        }else{
            async_uart_puts(input);
            async_uart_puts("\r\n");
        }
        disable_uart_interrupt();
    } else if (strcmp(command_string,"setTimeout")){
        if (para_idx < 2) {
            uart_puts("Usage: setTimeout MESSAGE SECONDS\n");
            return;
        }
        
        add_core_timer(print_core_timer_message,parameters[0],atoi(parameters[1])*get_core_frequency());
        add_core_timer(print_core_timer_message,"message1",1*get_core_frequency());
        add_core_timer(print_core_timer_message,"message2",10*get_core_frequency());
        add_core_timer(print_core_timer_message,"message3",5*get_core_frequency());


        enable_interrupt();
        core_timer_enable();

        int enable = 1;

        while (enable) {
            asm volatile("mrs %0, cntp_ctl_el0\r\n" :"=r"(enable));
        }
    }
}