#include "test.h"

#include "mem.h"
#include "scheduler.h"
#include "syscalls.h"
#include "timer.h"
#include "uart.h"

/* System Call Test */

static int get_pid() {
  int pid = -1;
  asm volatile(
      "mov x7, 255\n"
      "mov x8, 0\n"
      "svc 0\n"
      "mov %0, x0\n"
      : "=r"(pid)
      :
      : "x8", "x0");
  return pid;
}

static int fork() {
  int ret = -1;
  asm volatile(
      "mov x8, 4\n"
      "svc 0\n"
      "mov %0, x0\n"
      : "=r"(ret)
      :
      : "x8", "x0");
  return ret;
}

static void exit() {
  asm volatile(
      "mov x0, 0\n"
      "mov x8, 5\n"
      "svc 0\n");
}

void thread_test() {
  for (int i = 0; i < 10; ++i) {
    uart_log(TEST, "Thread id: ");
    uart_dec(get_current()->pid);
    uart_puts(" ");
    uart_dec(i);
    uart_putc(NEWLINE);

    int *timeup = 0;
    timer_add((void (*)(void *))set_timeup, (void *)timeup, 3);
    while (!*timeup);

    schedule();
  }
  kill_current_thread();
}

static void fork_test() {
  uart_log(TEST, "Fork Test, pid ");
  uart_dec(get_pid());
  uart_putc(NEWLINE);
  int cnt = 1;
  int ret = 0;
  if ((ret = fork()) == 0) {  // child
    long long cur_sp;         // force 64-bit aka uint64_t
    asm volatile("mov %0, sp" : "=r"(cur_sp));
    uart_log(TEST, "first child pid: ");
    uart_dec(get_pid());
    uart_puts(", cnt: ");
    uart_dec(cnt);
    uart_puts(", ptr: ");
    uart_hex((uintptr_t)&cnt);
    uart_puts(", sp: ");
    uart_hex(cur_sp);
    uart_putc(NEWLINE);
    cnt++;
    if ((ret = fork()) != 0) {
      asm volatile("mov %0, sp" : "=r"(cur_sp));
      uart_log(TEST, "first child pid: ");
      uart_dec(get_pid());
      uart_puts(", cnt: ");
      uart_dec(cnt);
      uart_puts(", ptr: ");
      uart_hex((uintptr_t)&cnt);
      uart_puts(", sp: ");
      uart_hex(cur_sp);
      uart_putc(NEWLINE);
    } else {
      while (cnt < 5) {
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        uart_log(TEST, "second child pid: ");
        uart_dec(get_pid());
        uart_puts(", cnt: ");
        uart_dec(cnt);
        uart_puts(", ptr: ");
        uart_hex((uintptr_t)&cnt);
        uart_puts(", sp: ");
        uart_hex(cur_sp);
        uart_putc(NEWLINE);
        int *timeup = 0;
        timer_add((void (*)(void *))set_timeup, (void *)timeup, 3);
        while (!*timeup);

        cnt++;
      }
    }
    exit();  // child
  } else {
    uart_log(TEST, "parent here, pid ");
    uart_dec(get_pid());
    uart_puts(", child ");
    uart_dec(ret);
    uart_putc(NEWLINE);
  }
  exit();  // parent
}

void run_fork_test() {
  if (get_current()->user_stack == 0) {
    uart_log(TEST, "Allocate user_stack for Fork Test\n");
    get_current()->user_stack = kmalloc(STACK_SIZE, 1);
  }
  asm volatile(  // el1 -> el0
      "msr spsr_el1, %0\n"
      "msr elr_el1, %1\n"
      "msr sp_el0, %2\n"
      "mov sp, %3\n"
      "eret\n"
      :
      : "r"(0x340),                                   // 0
        "r"(fork_test),                               // 1
        "r"(get_current()->user_stack + STACK_SIZE),  // 2
        "r"(get_current()->context.sp)                // 3
      : "memory");
}
