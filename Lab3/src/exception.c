#include "mini_uart.h"
#include "exception.h"

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

int lower_exception_entry(){
    uart_sends("lower exception entry!\n");

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