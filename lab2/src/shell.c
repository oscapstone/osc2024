#include "shell.h"
#include "mini_uart.h"

#include "utils.h"

//#include "mailbox.h"
#include "mbox.h"

#include "reboot.h"
#include "cpio.h"
#include "allocator.h"
#include "dtb.h"
#define COMMAND_MAX_LEN 256u

extern void *_dtb_ptr;

void read_command(char* command) {
	int index = 0;
	while(1) 
	{
		command[index] = uart_get_char();
		uart_send_char(command[index]);

		if(command[index] == '\n') 
		{
			command[index] = '\0';
			command[index+1] = '\n';
			break;
		}

		else if (command[index] == 127 ) //backspace
		{
			if (index > 0)
			{
				uart_send_char('\b');
				uart_send_char(' ');
				uart_send_char('\b');
				index--;
			}
		}

		else
		{
			index++;
		}
		
	}
}

void shell(){
  //char array_space[256];
  //char* input_string = array_space;
  	char command[COMMAND_MAX_LEN];

  	while(1) {
     	uart_send_string("# ");
		read_command(command);
		char * input_string = command;
		if(!strcmp(input_string,"help")) 
		{
			uart_send_string("\r");
			uart_send_string("help	:print this help menu\n\r");
			uart_send_string("hello	:print Hello World!\n\r");
			uart_send_string("info	:Get the hardware's information\n\r");
			uart_send_string("reboot	:reboot the device\n\r");
			uart_send_string("ls	:list the file\n\r");
			uart_send_string("cat	:print file content\n\r");
			uart_send_string("malloc	:give dynamic memory space\n\r");
			uart_send_string("dtb	:print device tree\n\n\r");
		} 
	 
		else if (!strcmp(input_string,"hello")) 
		{
			uart_send_string("\r");
			uart_send_string("Hello World!\n\n\r");
		} 
	 
		else if (!strcmp(input_string,"info")) 
		{
			uart_send_string("\r");
			get_board_revision();
        	get_mem_info();
		} 
	 
		else if (!strcmp(input_string,"reboot")) 
		{
			uart_send_string("\r");
			uart_send_string("Rebooting....\n\n\r");
			reset(1000);
		} 
	 
		else if (!strcmp(input_string,"ls")) 
		{
			uart_send_string("\r");
			cpio_ls();
		} 
	 
		else if (!strcmp(input_string,"cat"))
		{
			uart_send_string("\r");
			uart_send_string("Filename: ");
			char filename[COMMAND_MAX_LEN];
			read_command(filename);
			uart_send_string("\r");
			cpio_cat(filename);
		} 
	 
		else if (!strcmp(input_string,"malloc"))
		{
			uart_send_string("\r");
			char *a = simple_malloc(sizeof("9876"));
			char *b = simple_malloc(sizeof("345"));
			a[0] = '9';
			a[1] = '8';
			a[2] = '7';
			a[3] = '6';
			a[4] = '\0';
			b[0] = '3';
			b[1] = '4';
			b[2] = '5';
			b[3] = '\0';
			uart_send_string(a);
			uart_send_char('\n');
			uart_send_char('\r');
			uart_send_string(b);
			uart_send_char('\n\n');
			uart_send_char('\r');
		}	
	 
	 
		else if (!strcmp(input_string,"dtb")) 
		{
			uart_send_string("\r");
			fdt_traverse(print_dtb,_dtb_ptr);
		}  
	 
		else 
		{
			uart_send_string("\r");
			uart_send_string("The instruct is not exist.\n\n\r");
		}
  }
}
