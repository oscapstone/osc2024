#include "command.h"
#include "mini_uart.h"
#include "mailbox.h"
#include "initrd.h"
#include "string.h"
#include "heap.h"
#include "timer.h"

Command commands[] = {
    {"help", "print all available commands.", help},
    {"hello", "print Hello World!", hello},
    {"lshw", "show some hardware information.", lshw},
    {"reboot", "reboot the device.", reboot},
    {"ls", "list directory contents", ls},
    {"cat", "print on the standard output", cat},
    {"exec", "excution a user program", exec},
    {"settimeout", "prints MESSAGE after SECONDS with the current time and the command executed time", setTimeout},
    {"test", "test async read and write", test}
};

int help(void* args[]){
  for (int i = 0; i < MAX_COMM_NUM; i++)
  {
    uart_sends(commands[i].name);
    uart_sends("\t: ");
    uart_sends(commands[i].description);
    uart_sendc('\n');
  }
  return 0;
}

int hello(void* args[]){
  uart_sends("Hello world!\n");
  return 0;
}

int lshw(void* args[]){
  get_board_revision();
  get_arm_memory();
  return 0;
}

int reboot(void* args[]){
  *PM_RSTC = (PM_PASSWORD | 0x20);
  *PM_WDOG = (PM_PASSWORD | 20);
  return 0;
}

int ls(void* args[]){
  initrd_list();
  return 0;
}

int cat(void* args[]){
  if(!args){
    return -1;
  }
  char filename[257];
  strcpy(filename, args[1]);

  initrd_cat(filename);
  return 0;
}

int exec(void* args[]){
  if(!args){
    return -1;
  }
  char filename[257];
  strcpy(filename, args[1]);
  void* exec_data = fetch_exec(filename);

  if(exec_data){

    void *stack = simple_malloc(STACK_SIZE);
    void *stack_top = stack + STACK_SIZE;

    asm volatile(
      "mov x4, 0x3c0;" // 001111000000 
      "msr spsr_el1, x4;" // spsr_el1 bit[9:6] are D, A, I, and F; bit[3:0]: el0t
      "msr elr_el1, %0;"
      "msr sp_el0, %1;"
      "eret;" :: "r" (exec_data), "r" (stack_top) // :: is for input; : for output
    );
  }
  else{
    uart_sends("exec: ");
    uart_sends(filename);
    uart_sends(" No such file or directory\n");
  }
  return 0;
}

int setTimeout(void* args[]){
  if(!args){
    return -1;
  }
  set_timeout((char*)args[1], atoi(args[2]));
  return 0;
}

int test(void* args[]){
  uart_async_test();
  return 0;
}