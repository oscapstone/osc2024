#include "shell.h"
#include "uart.h"
#include "strcmp.h"
#include "mailbox.h"
#include "reboot.h"

void shell(){
    char buffer[256];
    // console can always run
    while(1){
    	uart_send_string("# ");
    	int index = 0;
    	// if type newline then break
    	// else keep typing
    	while(1){
    	    // char one by one
    	    buffer[index] = uart_get_char();
    	    uart_send_char(buffer[index]);
    	    // if newline
    	    if(buffer[index] == '\n'){
    	    	buffer[index] = '\0';
    	    	buffer[index + 1] = '\n';
    	    	break;
    	    }
    	    index++;
    	}
    	
    	char* input_string = buffer;
    	if(str_cmp(input_string, "help")){
    	    uart_send_string("help    : print this help menu\n");
    	    uart_send_string("hello   : print Hello World!\n");
    	    uart_send_string("info    : Get the hardware’s information\n");
    	    uart_send_string("reboot  : reboot the device\n");
    	}
    	else if(str_cmp(input_string, "hello")){
    	    uart_send_string("Hello World!\n");
    	}
    	else if(str_cmp(input_string, "info")){
    	    if(mailbox_call()){
		// board revision
		get_board_revision();
		uart_send_string("Board revision: ");
		bin_to_hex(mailbox[5]);
		uart_send_string("\r\n");
		// arm memory base
		get_arm_mem();
		uart_send_string("ARM memory base address: ");
		bin_to_hex(mailbox[5]);
		uart_send_string("\r\n");
		// arm memory size
		uart_send_string("ARM memory size: ");
		bin_to_hex(mailbox[6]);
		uart_send_string("\r\n");
	    }
	    else{
	        uart_send_string("error\n");
	    }
    	}
    	else if(str_cmp(input_string, "reboot")){
    	    uart_send_string("rebooting~~~~\n");
    	    reset(1000);
    	}
    }
}
