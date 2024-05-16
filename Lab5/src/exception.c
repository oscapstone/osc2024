#include "mini_uart.h"
#include "exception.h"
#include "interrupt.h"
#include "syscall.h"
#include "signal.h"
#include "thread.h"

int exception_entry(){
  uart_sends("exception entry!\n");

  unsigned long long spsr_el1, elr_el1, esr_el1;

  asm volatile(
    "mrs %0, spsr_el1;"
    "mrs %1, elr_el1;"
    "mrs %2, esr_el1;"
     : "=r" (spsr_el1), "=r" (elr_el1), "=r" (esr_el1)
  );

  uart_sends("spsr_el1: ");
  uart_sendh(spsr_el1);
  uart_sendc('\n');
  uart_sends("elr_el1: ");
  uart_sendh(elr_el1);
  uart_sendc('\n');
  uart_sends("esr_el1: ");
  uart_sendh(esr_el1);
  uart_sendc('\n');

  return 0;
}

int lower_exception_entry(trapframe *tf){
  // uart_sends("omg!\n");
  enable_irq();
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
    case 20:
      sys_sigreturn(tf->sp_el0);
      break;
    default:
      uart_sends("unknown syscall\n");
      break;
  }
  return 0;
}