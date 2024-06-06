#include "mini_uart.h"
#include "exception.h"
#include "interrupt.h"
#include "syscall.h"
#include "signal.h"
#include "thread.h"
#include "vm.h"

int exception_entry(){
  unsigned long long esr_el1;
  asm volatile("mrs %0, esr_el1;" : "=r" (esr_el1));
  int ec = esr_el1 >> 26;
  if(ec == 0b100001){
    // uart_sends("Instruction Abort\n");
    page_fault_handler();
  }
  else if(ec == 0b100101){
    // uart_sends("Data Abort\n");
    page_fault_handler();}
  else{
    uart_sends("exception entry!\n");
    unsigned long long spsr_el1, elr_el1, far_el1;

    asm volatile(
      "mrs %0, spsr_el1;"
      "mrs %1, elr_el1;"
      "mrs %2, far_el1;"
      : "=r" (spsr_el1), "=r" (elr_el1), "=r" (far_el1)
    );

    uart_sends("spsr_el1: ");
    uart_sendl(spsr_el1);
    uart_sendc('\n');
    uart_sends("elr_el1: ");
    uart_sendl(elr_el1);
    uart_sendc('\n');
    uart_sends("esr_el1: ");
    uart_sendl(esr_el1);
    uart_sendc('\n');
    uart_sends("far_el1: ");
    uart_sendl(far_el1);
    uart_sendc('\n');
    for(long long i = 0; i < 100000000; i++);
  }
  return 0;
}

int lower_exception_entry(trapframe *tf){
  unsigned long long esr_el1;
  asm volatile("mrs %0, esr_el1;" : "=r" (esr_el1));
  int ec = esr_el1 >> 26;

  enable_irq();
  if(ec == 0b010101){
    switch(tf->x[8]){
      case 0:
        tf->x[0] = sys_getpid();
        break;
      case 1:
        tf->x[0] = sys_uartread((char*)tf->x[0], tf->x[1]);
        break;
      case 2:
        tf->x[0] = sys_uartwrite((char*)tf->x[0], tf->x[1]);
        break;
      case 3:
        tf->x[0] = sys_exec((const char*)tf->x[0], (char* const*)tf->x[1]);
        break;
      case 4:
        disable_irq();
        sys_fork(tf);
        enable_irq();
        break;
      case 5:
        sys_exit();
        break;
      case 6:
        tf->x[0] = sys_mbox_call(tf->x[0], (unsigned int*)tf->x[1]);
        break;
      case 7:
        sys_kill(tf->x[0]);
        break;
      case 8:
        sys_signal(tf->x[0], (void(*)())tf->x[1]);
        break;
      case 9:
        posix_kill(tf->x[0], tf->x[1]);
        break;
      case 10:
        tf->x[0] = (unsigned long)sys_mmap((void*)tf->x[0], tf->x[1], tf->x[2], tf->x[3], tf->x[4], tf->x[5]);
        break;
      case 20:
        sys_sigreturn(tf->sp_el0);
        break;
      default:
        uart_sends("unknown syscall\n");
        break;
    }
  }
  else if(ec == 0b100000){
    // uart_sends("Instruction Abort\n");
    page_fault_handler();
  }
  else if(ec == 0b100100){
    // uart_sends("Data Abort\n");
    page_fault_handler();
  }
  else{
    uart_sends("Unknown exception: \n");
    unsigned long long spsr_el1, elr_el1, far_el1;

    asm volatile(
      "mrs %0, spsr_el1;"
      "mrs %1, elr_el1;"
      "mrs %2, far_el1;"
      : "=r" (spsr_el1), "=r" (elr_el1), "=r" (far_el1)
    );

    uart_sends("spsr_el1: ");
    uart_sendl(spsr_el1);
    uart_sendc('\n');
    uart_sends("elr_el1: ");
    uart_sendl(elr_el1);
    uart_sendc('\n');
    uart_sends("esr_el1: ");
    uart_sendl(esr_el1);
    uart_sendc('\n');
    uart_sends("far_el1: ");
    uart_sendl(far_el1);
    uart_sendc('\n');
    for(long long i = 0; i < 100000000; i++);
  }
  return 0;
}