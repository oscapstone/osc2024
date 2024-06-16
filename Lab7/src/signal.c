#include "mini_uart.h"
#include "thread.h"
#include "signal.h"
#include "string.h"
#include "syscall.h"
#include "memory.h"

void signal_check(){
  thread *t = get_current();
  if(t->signal_pending){
    for(int i = 0; i < MAX_SIGNAL+1; i++){
      if(t->signal_pending & (1 << i)){
        t->signal_pending &= ~(1 << i);
        signal_exec(t, i);
      }
    }
  }
}

void signal_exec(thread *t, int signum){
  if(t->signal_processing)
    return;
  t->signal_processing = 1;

  if(!t->signal_handler[signum]){
    default_signal_handler();
    return;
  }

  memcpy((void*)&t->signal_regs, (void*)&t->regs, sizeof(callee_reg));

  // set new context for signal handler
  // t->regs.sp = virt_to_phys(kmalloc(THREAD_STACK_SIZE));
  void* signal_stack = (void*)virt_to_phys(kmalloc(THREAD_STACK_SIZE));
  add_vma(&t->vma_list, 0x120000, virt_to_phys(signal_stack), THREAD_STACK_SIZE, 0b111);
  t->regs.lr = 0x100000; // virtual address of handler_container

  asm volatile(
    "msr spsr_el1, %0;"
    "msr elr_el1, %1;"
    "msr sp_el0, %2;"
    "mov x0, %3;"
    "eret;" :: "r"(0x0), "r" (t->regs.lr+((unsigned long)handler_container % 0x1000)), "r" (0x124000), "r"(t->signal_handler[signum])
  );
}

void handler_container(void (*handler)()){
  handler();
  sigreturn();
}

void default_signal_handler(){
  sys_kill(get_current()->tid);
}
