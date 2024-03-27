#include "shell.h"
#include "uart.h"
#include "mailbox.h"
#include "utils.h"
#include "reboot.h"
#include "cpio.h"
#include "allocator.h"
#include "dtb.h"

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
    uart_display_string("\r");
    uart_display_string(">>");
    read_command(buffer);

    
    char * input_string = buffer;

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
  }
}