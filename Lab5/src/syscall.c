#include "exception.h"
#include "initrd.h"
#include "string.h"
#include "mailbox.h"
#include "mini_uart.h"
#include "interrupt.h"
#include "memory.h"
#include "syscall.h"
#include "thread.h"

extern void restore_context(callee_reg* regs);

int sys_getpid(){
  thread *t = get_current();
  return t->tid;
}

size_t sys_uartread(char buf[], size_t size){
  size_t i = 0;
  while(i < size){
    buf[i] = uart_recv();
    i++;
  }
  return i;
}

size_t sys_uartwrite(char buf[], size_t size){
  size_t i;
  for(i = 0; i < size; i++){
    uart_sendc(buf[i]);
  }
  return i;
}

int sys_exec(const char *name, char *const argv[]){
  void* exec = fetch_exec((char*)name);
  int program_size = get_exec_size(name);

  void *p = kmalloc(program_size);

  // for(int i = 0; i < program_size; i++){
  //   *((char*)p+i) = *((char*)exec+i);
  // }
  memcpy((void*)p, (void*)exec, program_size);

  thread *t = get_current();
  for(int i = 0; i < MAX_SIGNAL+1; i++){
    t->signal_handler[i] = 0;
  }
  t->regs.lr = (unsigned long)p;

  asm volatile(
    "msr tpidr_el1, %0;"
    "mov x4, 0x0;" // 001111000000
    "msr spsr_el1, x4;" // spsr_el1 bit[9:6] are D, A, I, and F; bit[3:0]: el0t
    "msr elr_el1, %1;"
    "msr sp_el0, %2;"
    "mov sp, %3;"
    "eret;" :: "r" (t), "r" (t->regs.lr), "r" (t->regs.sp), "r" (t->kernel_stack+0x1000)
  );
  return 0;
}

void set_child_lr(thread* t){
  unsigned long ret;
  asm volatile("mov %0, lr" : "=r" (ret));
  t->regs.lr = ret;
  return;
}

// when do system call, the thread->regs should store kernel state.
int sys_fork(trapframe *tf){
  thread *cur = get_current();
  thread *child = thread_create(0);
  int child_tid = child->tid;

  // user stack copy
  memcpy((void*)child->stack, (void*)cur->stack, THREAD_STACK_SIZE);
  
  // kernel stack copy
  memcpy((void*)child->kernel_stack, (void*)cur->kernel_stack, THREAD_STACK_SIZE);
  
  // signal handler copy
  for(int i = 0; i < MAX_SIGNAL+1; i++){
    child->signal_handler[i] = cur->signal_handler[i];
  }

  unsigned long cur_sp;
  asm volatile("mov %0, sp" : "=r"(cur_sp));
  child->regs.sp = (unsigned long)(cur_sp + ((void*)child->kernel_stack - cur->kernel_stack));

  trapframe *child_tf = (trapframe *)(child->kernel_stack + ((char*)tf - (char*)cur->kernel_stack));

  child_tf->x[0] = 0;
  child_tf->sp_el0 = (unsigned long)(child->stack+((void*)tf->sp_el0 - cur->stack));
  set_child_lr(child);

  if(get_current()->tid == child_tid){
    return 0;
  }

  tf->x[0] = child_tid;
  return child->tid;
}

int sys_mbox_call(unsigned char ch, unsigned int *mbox){
  return mailbox_call(ch, mbox);
}

void sys_exit(int status){
  thread_exit();
}

void sys_kill(int pid){
  thread_kill(pid);
}

void sys_signal(int signum, void (*handler)()){
  thread *t = get_current();
  t->signal_handler[signum] = handler;
}

void posix_kill(int pid, int signum){
  thread *t = get_thread(pid);
  if(t){
    t->signal_pending |= 1 << signum;
  }
}

void sys_sigreturn(unsigned long sp_el0){
  thread *t = get_current();
  kfree((void*)((sp_el0 % THREAD_STACK_SIZE)?(sp_el0 & ~(THREAD_STACK_SIZE-1)):(sp_el0 - THREAD_STACK_SIZE)));
  restore_context(&t->signal_regs);
}

int getpid(){
  int ret;
  asm volatile("mov x8, 0;"
               "svc	0;"
               "mov	%0 , x0;"
               : "=r" (ret));
  return ret;
}

int fork(){
  int ret;
  asm volatile("mov x8, 4;"
               "svc	0;"
               "mov	%0 , x0;"
               : "=r" (ret));
  return ret;
}

void exit(int status){
  asm volatile("mov x8, 5;"
               "svc	0;");
}

void sigreturn(){
  asm volatile(
    "mov x8, 20;"
    "svc 0;"
  );
}

/* For test */

void jump_to_el0(){
  thread *t = get_current();
  uart_sendi(t->tid);
  for(int i = 0; i < 1000000; i++);

  asm volatile("msr spsr_el1, %0" ::"r"(0x3c0));          // disable E A I F
  asm volatile("msr elr_el1, %0" ::"r"(fork_test));       // get back to caller function
  asm volatile("msr sp_el0, %0" ::"r"(t->regs.sp));
  asm volatile("mov sp, %0" ::"r"(t->kernel_stack + THREAD_STACK_SIZE));
  asm volatile("eret");
}

void fork_test(){
    uart_sends("\nFork Test, pid ");

    uart_sendi(getpid());
    uart_sends("\n");
        for(int i = 0; i < 1000000; i++); // delay
    int cnt = 1;
    int ret = 0;
    if ((ret = fork()) == 0) { // child
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        // printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", get_pid(), cnt, &cnt, cur_sp);
        uart_sends("first child pid: ");
        uart_sendi(getpid());
        uart_sends(", cnt: ");
        uart_sendi(cnt);
        uart_sends(", ptr: ");
        uart_sendh((unsigned long)&cnt);
        uart_sends(", sp: ");
        uart_sendh(cur_sp);
        uart_sends("\n");
        ++cnt;

        if ((ret = fork()) != 0){
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            // printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", get_pid(), cnt, &cnt, cur_sp);
            uart_sends("first child pid: ");
            uart_sendi(getpid());
            uart_sends(", cnt: ");
            uart_sendi(cnt);
            uart_sends(", ptr: ");
            uart_sendh((unsigned long)&cnt);
            uart_sends(", sp: ");
            uart_sendh(cur_sp);
            uart_sends("\n");
        }
        else{
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                // printf("second child pid: %d, cnt: %d, ptr: %x, sp : %x\n", get_pid(), cnt, &cnt, cur_sp);
                uart_sends("second child pid: ");
                uart_sendi(getpid());
                uart_sends(", cnt: ");
                uart_sendi(cnt);
                uart_sends(", ptr: ");
                uart_sendh((unsigned long)&cnt);
                uart_sends(", sp: ");
                uart_sendh(cur_sp);
                uart_sends("\n");
                ++cnt;
            }
        }
        exit(0);
    }
    else {
        // printf("parent here, pid %d, child %d\n", get_pid(), ret);
        uart_sends("parent here, pid ");
        uart_sendi(getpid());
        uart_sends(", child ");
        uart_sendi(ret);
        uart_sends("\n");
        exit(0);
    }
}

void sys_fork_test(){
  thread_create(jump_to_el0);
  idle();
}
