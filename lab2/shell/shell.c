#include "header/shell.h"
#include "header/uart.h"
#include "header/utils.h"
#include "header/mailbox.h"
#include "header/cpio.h"
#include "header/allocator.h"
#include "header/dtb.h"
extern void *_dtb_ptr;
// extern char * cpio_addr;

void read_command(char* buffer) {
	int index = 0;
	while(1) {
		buffer[index] = uart_get_char();
		if(buffer[index] == '\n') {
			buffer[index] = '\0';
			// buffer[index+1] = '\n';
			//uart_send_string("\n");
			break;
		}
		else 
		{
			uart_send_char(buffer[index]);
		}
		index++;
	}
}

void shell(){
  char input_space[256];
  char* input_string = input_space;
  while(1) {
     char element;
     uart_send_string("# ");
     while(1) {
       element = uart_get_char();
       *input_string++ = element;
       if(element == '\n')
       {
        uart_send_string("\r\n");
        *input_string = '\0';
        break;
       }
       else
       {
          uart_send_char(element);
       }
     }
     
     input_string = input_space;
     if(ut_st_comp(input_string,"help")) 
     {
       uart_send_string("help	:print this help menu\n");
       uart_send_string("hello	:print Hello World!\n");
       uart_send_string("info	:Get the hardware's information\n");
       uart_send_string("ls	:list the file\n");
       uart_send_string("cat	:print file content\n");
       uart_send_string("malloc	:give dynamic memory space\n");
       uart_send_string("dtb	:print device tree\n");
     } 
     else if (ut_st_comp(input_string,"hello")) 
     {
       uart_send_string("Hello!\n");
       // uart_send_hex((uintptr_t) _dtb_ptr);
       // uart_send_string("\n");
       // uart_send_hex((uintptr_t) cpio_addr);
       // uart_send_string("\n");
     } 
     else if (ut_st_comp(input_string,"info")) 
     {
        get_board_revision();
        get_arm_mem();
     } 
     else if (ut_st_comp(input_string,"ls")) 
     {
        uart_send_string("in ls\n");
        cpio_ls();
     }
     else if (ut_st_comp(input_string,"cat")) 
     {
        uart_send_string("Filename: ");
        char filename[32];
        read_command(filename);
        uart_send_string(filename);
        uart_send_string("\n");
        cpio_cat(filename);
        //uart_send_string(filename);
		   //char filename[BUFFER_MAX_SIZE];
		   //read_command(filename);
           //uart_send_string(filename);
		   //cpio_cat(filename);
     }
     else if (ut_st_comp(input_string,"malloc")) 
     {
		char *a = mem_alloc(sizeof("9876"));
		char *b = mem_alloc(sizeof("345"));
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
		uart_send_string("\n");
	    uart_send_string(b);
        uart_send_string("\n");
     }
     else if (ut_st_comp(input_string,"dtb")) 
     {
        fdt_traverse(print_dtb,_dtb_ptr);
     }            
  }
}
