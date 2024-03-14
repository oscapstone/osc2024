#include "header/shell.h"
#include "header/uart.h"
#include "header/utils.h"
#include "header/mailbox.h"
#include "header/reboot.h"

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
     if(string_compare(input_string,"help")) 
     {
       uart_send_string("help	:print this help menu\n");
       uart_send_string("hello	:print Hello World!\n");
       uart_send_string("info	:Get the hardware's information\n");
       uart_send_string("reboot	:reboot the device\n");
     } 
     else if (string_compare(input_string,"hello")) 
     {
       uart_send_string("Hello World!\n");
     } 
     else if (string_compare(input_string,"info")) 
     {
        get_board_revision();
        get_arm_mem();
     } 
     else if (string_compare(input_string,"reboot")) {
        uart_send_string("Rebooting....\r\n");
        reset(1000);
        break;
     }     
  }
}
