#include "shell.h"
#include "uart.h"
#include "mailbox.h"
#include "utils.h"
#include "reboot.h"
#include "cpio.h"
#include "allocator.h"
#include "dtb.h"
#include "interrupt.h"
#include "time_c.h"
#include "printf.h"

extern void *_dtb_ptr;

#define BUFFER_MAX_SIZE 256u

void read_command(char* buffer) {
    int index = 0;
    while(1) {
        buffer[index] = uart_get_char();
        uart_send_char(buffer[index]);
        if(buffer[index] == '\n') {
          buffer[index] = '\0';
          buffer[index+1] = '\n';
          break;
        }
        index++;
    }
}

void shell(){
    
    char buffer[BUFFER_MAX_SIZE];
    

    while(1) {
        uart_buf_init();
        
        
        
        uart_display_string("\r");
        uint32_t now;
        asm volatile("adr %0,#0":"=r"(now));
        printf("0x%x",now);
        uart_display_string(">>");
        read_command(buffer);
        
        char * input_string = buffer;

        if(!utils_find_api(input_string)){

            if(utils_string_compare(input_string,"help")) {
                uart_display_string("\r");
                uart_display_string("help	: Print this help menu\n");
                uart_display_string("hello : Print Hello World!\n");
                uart_display_string("info	: Get the hardware's information\n");
                uart_display_string("reboot	: Reboot the device\n");
                uart_display_string("ls : List the file\n");
                uart_display_string("cat : show a file content\n");
                uart_display_string("malloc : give dynamic memory space\n");
                uart_display_string("dtb : read dtb\n");
                uart_display_string("exe : execute the user program & print Spec-info \n");
                uart_display_string("coretimer : coretimer testing\n");
                uart_display_string("async_print : print the message out of sync\n");
                uart_display_string("async_get : get the message out of sync and print the message\n");
                uart_display_string("addtimer(msg, time) : the message will print after five seconds\n");
            } 
            else if (utils_string_compare(input_string,"hello")) {
                uart_display_string("\rHello World!\n");
            } 
            else if (utils_string_compare(input_string,"info")) {
                get_board_revision();
                if (mailbox_call()) { 
                    uart_display_string("\rMy board revision is: ");
                    uart_binary_to_hex(mailbox[5]);
                    uart_display_string("\n");
                }
                else {
                    uart_display_string("Unable to query serial!\n");
                }

                get_arm_mem();
                if(mailbox_call()) {
                    uart_display_string("My ARM memory base address is: ");
                    uart_binary_to_hex(mailbox[5]);
                    uart_display_string("\n");
                    uart_display_string("My ARM memory size is: ");
                    uart_binary_to_hex(mailbox[6]);
                    uart_display_string("\n");
                } 
                else {
                    uart_display_string("Unable to query serial!\n");
                } 
            }
            else if (utils_string_compare(input_string,"reboot")) {
                uart_display_string("Rebooting....\n");
                reset(1000);
            }
            else if (utils_string_compare(input_string,"ls")) {
                cpio_ls();
            }
            else if (utils_string_compare(input_string,"cat")){
                uart_send_char('\r');
                uart_display_string("Filename: ");
                char filename[BUFFER_MAX_SIZE];
                read_command(filename);
                cpio_cat(filename);
            }
            else if (utils_string_compare(input_string,"malloc")){
                char *a = simple_malloc(sizeof("allocater"));
                char *b = simple_malloc(sizeof("345"));
                a[0] = 'a'; a[1] = 'l'; a[2] = 'l'; a[3] = 'o';
                a[4] = 'c'; a[5] = 't'; a[6] = 'a'; a[7] = 'r';
                a[8] = '\0';
                b[0] = '3'; b[1] = '4'; b[2] = '5';
                b[3] = '\0';
                uart_send_char('\r');
                uart_display_string(a);
                uart_send_char('\n');
                uart_send_char('\r');
                uart_display_string(b);
                uart_send_char('\n');	 
            }
            else if (utils_string_compare(input_string,"dtb")){
                fdt_traverse(print_dtb, _dtb_ptr);
            }
            else if (utils_string_compare(input_string, "exe")){
                uart_display_string("program name: ");
                char buffer[BUFFER_MAX_SIZE];
                read_command(buffer);
                cpio_load_program(buffer);
            }

            else if (utils_string_compare(input_string, "async_print") || utils_string_compare(input_string, "ap")){
                uart_display_string("\r");
                uart_async_send_string("|---------->async test<----------|");
                uart_async_send_string("|    |    |  o                   |");
                uart_async_send_string("|    |----|  |                   |");
                uart_async_send_string("|    |    |  |   ...........     |");
                uart_async_send_string("|--------------------------------|");
            }
            else if (utils_string_compare(input_string, "async_get")){
                test_uart_async();
            }
            else if (utils_string_compare(input_string, "3timer")){
                setTimeout("This is 10", 10);
                setTimeout("This is 5", 5);
                setTimeout("This is 4", 4);
                setTimeout("execute", 0);
            }
            else{
                uart_display_string("\rNo instruction\n");
            }
        }
        else{
            if(utils_api_compare(input_string,"addtimer()")){
                

                char* api_buff = (char*) simple_malloc(sizeof(input_string));
                char* message = (char*) simple_malloc(sizeof(input_string));
                char* delaytime = (char*) simple_malloc(sizeof(input_string));

                api_buff = utils_api_analysis(input_string);

                utils_api_get_elem(api_buff, &message, &delaytime);

                char*msg = (char*) simple_malloc(sizeof(message));
                for(int i=0;i<=sizeof(message);i++){
                    msg[i]=message[i];
                }
                
                setTimeout(msg,utils_str2int(delaytime));
                setTimeout("api executing",0);
            }
            else{
                uart_display_string("\rYou input a INVAILD function\n");
            }
        }
    }
}