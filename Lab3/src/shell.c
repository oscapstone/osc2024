#include "shell.h"
#include "mini_uart.h"
#include "command.h"
#include "string.h"

#define BUF_LEN 256

void shell_exec(){
  char buffer[BUF_LEN + 1];
  char *tmp;

  while (1){
    uart_sends("# ");
    tmp = buffer;
    for (int i = 0; i < BUF_LEN; i++)
    {
      *tmp = uart_recv();
      uart_sendc(*tmp);
      if (*tmp == 127){
        i--;
        if(i >= 0){
          i--;
          *tmp-- = 0;
          uart_sendc('\b');
          uart_sendc(' ');
          uart_sendc('\b');
        }
      }
      else if (*tmp == '\n'){
        break;
      }
      else tmp++;
    }
    *tmp = '\0';
    command_exec(buffer);
  }
}

void command_exec(const char *s){
  // uart_sends(s);
  for (int i = 0; i < MAX_COMM_NUM; i++)
  {
    if (!strcmp(s, commands[i].name)){
      commands[i].func();
      return;
    }
  }
  uart_sends(s);
  uart_sends(": command not found\n");
}