#include "command.h"
#include "mini_uart.h"
#include "mailbox.h"
// #define MAX_COMM_NUM 4

struct Command commands[] = {
    {"help", "print all available commands.", help},
    {"hello", "print Hello World!", hello},
    {"lshw", "show some hardware information.", lshw},
    {"reboot", "reboot the device.", reboot}
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

