#include "command.h"
#include "mini_uart.h"
#include "mailbox.h"
#include "memory.h"
#include "initrd.h"
#include "string.h"
#include "heap.h"
#include "interrupt.h"
#include "timer.h"
#include "thread.h"
#include "syscall.h"
#include "vm.h"
#include "signal.h"

Command commands[] = {
    {"help", "print all available commands.", help},
    {"hello", "print Hello World!", hello},
    {"lshw", "show some hardware information.", lshw},
    {"reboot", "reboot the device.", reboot},
    {"ls", "list directory contents", ls},
    {"cat", "print on the standard output", cat},
    {"exec", "excution a user program", exec},
    {"settimeout", "prints MESSAGE after SECONDS with the current time and the command executed time", setTimeout},
    {"test", "demo test", test}
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
    unsigned int size = get_exec_size();
    void *p = kmalloc(size);
    memcpy((void*)p, exec_data, size);
    thread *t = thread_create(p);
    t->stack = kmalloc(THREAD_STACK_SIZE);

    add_vma(&t->vma_list, 0x0, virt_to_phys(p), size, 0b111);
    add_vma(&t->vma_list, 0xffffffffb000, virt_to_phys(t->stack), 0x4000, 0b111);
    add_vma(&t->vma_list, 0x3c000000, 0x3c000000, 0x3000000, 0b111);
    add_vma(&t->vma_list, 0x100000, virt_to_phys(handler_container), 0x2000, 0b101);
    t->regs.lr = 0x0;
    t->regs.sp = 0xfffffffff000;
    t->regs.fp = t->regs.sp;
    asm volatile(
      "msr tpidr_el1, %0;"
      "msr spsr_el1, xzr;" // spsr_el1 bit[9:6] are D, A, I, and F; bit[3:0]: el0t
      "msr elr_el1, %1;"
      "msr sp_el0, %2;"
      "dsb ish;"
      "msr ttbr0_el1, %3;"
      "tlbi vmalle1is;"
      "dsb ish;"
      "isb;"
      "mov sp, %4;"
      "eret;" :: "r" (t), "r" (t->regs.lr), "r" (t->regs.sp), "r"(t->regs.pgd), "r" (t->kernel_stack+0x4000)
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
  uart_sends("demo option:\n");
  uart_sends("1. uart_async_test\n");
  uart_sends("2. thread_test\n");
  uart_sends("3. fork_test\n");
  uart_sends("> ");
  char opt[256];
  gets(opt);
  switch(atoi(opt)){
    case 1:
      uart_async_test();
      break;
    case 2:
      thread_test();
      break;
    case 3:
      sys_fork_test();
      break;
  }

  return 0;
}
