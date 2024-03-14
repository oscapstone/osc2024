#include "command.h"
#include "mini_uart.h"
#include "mailbox.h"
#include "initrd.h"

Command commands[] = {
    {"help", "print all available commands.", help},
    {"hello", "print Hello World!", hello},
    {"lshw", "show some hardware information.", lshw},
    {"reboot", "reboot the device.", reboot},
    {"ls", "list directory contents", ls},
    {"cat", "print on the standard output", cat}
};

// enum Type inputToType(const char* command){

// }

int help(){
  for (int i = 0; i < MAX_COMM_NUM; i++)
  {
    uart_sends(commands[i].name);
    uart_sends("\t: ");
    uart_sends(commands[i].description);
    uart_sendc('\n');
  }
  return 0;
}

int hello(){
  uart_sends("Hello world!\n");
  return 0;
}

int lshw(){
  get_board_revision();
  get_arm_memory();
  return 0;
}

int reboot(){
  // uart_writeS("Re\n");
  *PM_RSTC = (PM_PASSWORD | 0x20);
  *PM_WDOG = (PM_PASSWORD | 20);
  return 0;
}

int ls(){
  initrd_list();
  return 0;
}

int cat(){
  uart_sends("Filename: ");
  char filename[257];
  char *tmp = filename;
  for (int i = 0; i < 256; i++)
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
  
  initrd_cat(filename);
  return 0;
}