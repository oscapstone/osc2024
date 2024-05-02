#include "shell.h"
#include "mini_uart.h"
#include "command.h"
#include "string.h"
#include "timer.h"

#include <stddef.h>

#define BUF_LEN 256

void shell_exec(){
  char buffer[BUF_LEN];
  memset(buffer, 0, BUF_LEN);
  char *args[10];

  while (1){
    int argc = 0;
    uart_sends("# ");
    gets(buffer);
    if(!strcmp(buffer, ""))
      continue;
    char *token = strtok(buffer, " ");
    while(token){
      args[argc++] = token;
      token = strtok(NULL, " ");
    }

    command_exec(argc, args);
  }
}

void command_exec(int argc, char* args[]){
  for (int i = 0; i < MAX_COMM_NUM; i++)
  {
    if (!strcmp(args[0], commands[i].name)){
      if(argc == 1)
        commands[i].func(NULL);
      else commands[i].func((void*)args);
      return;
    }
  }
  uart_sends(args[0]);
  uart_sends(": command not found\n");
}