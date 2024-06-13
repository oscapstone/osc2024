/*
 * Copyright (C) 2018 bzt (bztsrc@github)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */
#define MAX_BUFFER 50
#include "include/uart.h"
#include "include/utils.h"
#include "include/mbox.h"
#include "include/cpio.h"


extern void set_exception_vector_table();
extern void core_timer_enable();



void main(int argc, char* argv[]){
   uart_init();
   set_exception_vector_table();
   //cpio_exec("user_program.img");   
   core_timer_enable();
   while(1){
      char command[MAX_BUFFER];
      char c = '\0';
      int length=0;
      for(int i=0; i<MAX_BUFFER; i++){
         command[i] = '\0';
      }
      for(length=0; c != '\n' && length<MAX_BUFFER; length++){
         c = uart_getc();
         command[length] = c;
         // print what user input to screen
         // uart_send(c);
      }
      command[length==MAX_BUFFER?length-2:length-1] = '\0';
      if(strcmp(command, "help") == 0){
         uart_puts("help\t: print this help menu\n");
         uart_puts("ls\t: list all files in the root filesystem\n");
         uart_puts("cat <filename>\t: print the content of file in root filesystem\n");
         uart_puts("hello\t: print hello world!\n");
         uart_puts("reboot\t: reboot the device\n");
      }
      else if(strcmp(command, "hello") == 0){
         uart_puts("Hello World!\n");
      }
      else if(strcmp(command, "ls") == 0){
         cpio_ls();
      }
      else if(strcmp(command, "reboot") == 0){
         reset();
      }
      else if(strcmp(command, "exec") == 0){
         uart_puts("executing...\n");
         // execute a dedicated user program
         cpio_exec("user_program.img");
      }
      // command with arguments
      else{
         char arguments[3][MAX_BUFFER];
         // initialize
         for(int i=0;i<3;i++){
            for(int j=0;j<MAX_BUFFER;j++){
               arguments[i][j] = '\0';
            }
         }
         int argument_index =0;
         for(int j=0, indicater = 0; ; j++, indicater++){
            if(command[j] == ' '){
               arguments[argument_index][indicater+1] = '\0';
               argument_index++;
               indicater=-1;
            }
            else if(command[j] == '\0'){
               arguments[argument_index][indicater+1] = '\0';
               break;
            }
            else{
               arguments[argument_index][indicater]=command[j];
            }
         }
         // invalid input
         if(argument_index == 0){
            uart_puts("invalid\n");
         }
         if(strcmp(arguments[0], "cat") == 0){
            cpio_cat(arguments[1]);
         }
      }
   }
   return;
}
