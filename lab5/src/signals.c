#include "signals.h"

#include "mem.h"
#include "scheduler.h"
#include "syscalls.h"
#include "traps.h"
#include "uart.h"
#include "utils.h"

void signal(int SIGNAL, void (*handler)()) {
  get_current()->sig_handlers[SIGNAL] = handler;
}

void signal_kill(int pid, int SIGNAL) {
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
        kill_current_thread();  // Default handler: (exit the process)
        get_current()->sig_busy = 0;
        return;  // Jump to the previous context (user program) after eret
      }

      // Save the sigframe
      get_current()->sig_tf = kmalloc(sizeof(trap_frame), 0);
      memcpy(get_current()->sig_tf, tf, sizeof(trap_frame));
      get_current()->sig_stack = kmalloc(STACK_SIZE, 0);
      // will be releases in sys_sigreturn()
      tf->x30 = (uintptr_t)
          sigreturn;  // Return to sigreturn (traps.S) after signal handling
      tf->spsr_el1 = 0x340;
      tf->elr_el1 = (unsigned long)get_current()->sig_handlers[sig_num];
      tf->sp_el0 = (unsigned long)get_current()->sig_stack + STACK_SIZE;
      return;  // eret to the signal handler (->el0)
    }
    sig_num++;
  }
}