#include "shell.h"
#include "uart.h"
#include "mailbox.h"
#include "utils.h"
#include "reboot.h"

void shell(){
  char array_space[256];
  char* input_string = array_space;
  while(1) {
    char element;
    uart_display_string(">>");
    while(1) {
      element = uart_get_char();
      if(element == 127){
        continue;
      }
      else{
        *input_string++ = element;
      }
      
      uart_send_char(element);
      if(element == '\n'){
        *input_string = '\0';
        break;
      }
    }
    uart_display_string("\r");
    // uart_display_string(array_space);
    input_string = array_space;
    if(string_compare(input_string,"help")) {
      uart_display_string("help	:Print this help menu\n");
      uart_display_string("hello :Print Hello World!\n");
      uart_display_string("info	:Get the hardware's information\n");
      uart_display_string("reboot	:Reboot the device\n");
    } 
    else if (string_compare(input_string,"hello")) {
      uart_display_string("Hello World!\n");
    } 
    else if (string_compare(input_string,"info")) {
      get_board_revision();
      if (mailbox_call()) { 
        uart_display_string("My board revision is: ");
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
    else if (string_compare(input_string,"reboot")) {
           uart_display_string("Rebooting....\n");
           reset(1000);
    }    
  }
}