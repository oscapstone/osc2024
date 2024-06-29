#include "signals.h"

#include "mem.h"
#include "scheduler.h"
#include "syscalls.h"
#include "traps.h"
#include "uart.h"
#include "utils.h"
#include "virtm.h"

void signal(int SIGNAL, void (*handler)()) {
  if (SIGNAL < 1 || SIGNAL > NSIG) {
    uart_log(ERR, "Invalid signal number.\n");
    return;
  }
  uart_log(INFO, "Signal handler set for signal ");
  uart_dec(SIGNAL);
  uart_putc(NEWLINE);
  get_current()->sig_handlers[SIGNAL] = handler;
}

void signal_kill(int pid, int SIGNAL) {
  uart_log(INFO, "Signal ");
  uart_dec(SIGNAL);
  uart_puts(" sent to pid ");
  uart_dec(pid);
  uart_putc(NEWLINE);
  thread_struct *thread = get_thread_by_pid(pid);
  if (thread == 0) {
    uart_log(WARN, "No one gets the signal.\n");
    return;
  };
  thread->sig_reg |= 1 << (SIGNAL - 1);  // Set the signal pending bit
}

void do_signal(trap_frame *tf) {
  // Prevent nested signal handling
  if (get_current()->sig_busy) return;
  int sig_num = 1;
  while (get_current()->sig_reg) {
    if (get_current()->sig_reg & (0x1 << (sig_num - 1))) {
      get_current()->sig_busy = 1;  // block other signal handling
      get_current()->sig_reg &= ~(0x1 << (sig_num - 1));

      if (get_current()->sig_handlers[sig_num] == 0) {
        kill_current_thread();  // Default handler: kill the process
        return;
      }

      // Save the sigframe
      memcpy(get_current()->sig_tf = kmalloc(sizeof(trap_frame), SILENT), tf,
             sizeof(trap_frame));
      get_current()->sig_stack =
          kmalloc(STACK_SIZE, SILENT);  // free at sys_sigreturn

      // Map the stack for signal handling
      map_pages((unsigned long)get_current()->pgd, SIG_STACK, STACK_SIZE,
                (unsigned long)TO_PHYS(get_current()->sig_stack));

      tf->x30 = (uintptr_t)sigreturn;  // Return to sigreturn (traps.S) after
      tf->spsr_el1 = 0x340;
      tf->elr_el1 = (unsigned long)get_current()->sig_handlers[sig_num];
      tf->sp_el0 = (unsigned long)SIG_STACK + STACK_SIZE;  // top of sig_stack
      return;  // eret to the signal handler (->el0)
    }
    sig_num++;
  }
}