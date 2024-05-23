#include <stdarg.h>
#include <stdint.h>

typedef volatile char* addr_t;

#define MMIO_BASE      0x3F000000
#define AUX_MU_IO_REG  ((addr_t)(MMIO_BASE + 0x00215040))
#define AUX_MU_LSR_REG ((addr_t)(MMIO_BASE + 0x00215054))
#define NOP            asm volatile("nop")

int main();
void _start() {
  main();
}

#include "nanoprintf.hpp"
#define NANOPRINTF_IMPLEMENTATION
#include "nanoprintf/nanoprintf.h"

long int syscall(long int number, ...) {
  va_list args;

  va_start(args, number);
  register long int x8 asm("x8") = number;
  register long int x0 asm("x0") = va_arg(args, long int);
  register long int x1 asm("x1") = va_arg(args, long int);
  register long int x2 asm("x2") = va_arg(args, long int);
  register long int x3 asm("x3") = va_arg(args, long int);
  register long int x4 asm("x4") = va_arg(args, long int);
  register long int x5 asm("x5") = va_arg(args, long int);
  va_end(args);

  asm volatile("svc\t0"
               : "=r"(x0)
               : "r"(x8), "r"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5));
  return x0;
}

int uartread(const char buf[], unsigned size) {
  return syscall(1, buf, size);
}

int uartwrite(const char buf[], unsigned size) {
  return syscall(2, buf, size);
}

char getc() {
  char c = 0;
  while (uartread(&c, 1) != 1)
    ;
  return c;
}

void putc(char c) {
  uartwrite(&c, 1);
}

void mini_uart_npf_putc(int c, void* /* ctx */) {
  putc(c);
}

int printf(const char* format, ...) {
  va_list args;
  va_start(args, format);
  int size = npf_vpprintf(&mini_uart_npf_putc, NULL, format, args);
  va_end(args);
  return size;
}

inline void delay(unsigned cycle) {
  while (cycle--)
    NOP;
}

int get_pid() {
  return syscall(0);
}

int fork() {
  return syscall(4);
}

void exit(int status) {
  syscall(5, status);
}

int main() {
  printf("\nFork Test, pid %d\n", get_pid());
  int cnt = 1;
  int ret = 0;
  if ((ret = fork()) == 0) {  // child
    long long cur_sp;
    asm volatile("mov %0, sp" : "=r"(cur_sp));
    printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", get_pid(), cnt,
           &cnt, cur_sp);
    ++cnt;

    if ((ret = fork()) != 0) {
      asm volatile("mov %0, sp" : "=r"(cur_sp));
      printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", get_pid(), cnt,
             &cnt, cur_sp);
    } else {
      while (cnt < 5) {
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        printf("second child pid: %d, cnt: %d, ptr: %x, sp : %x\n", get_pid(),
               cnt, &cnt, cur_sp);
        delay(1000000);
        ++cnt;
      }
    }
  } else {
    printf("parent here, pid %d, child %d\n", get_pid(), ret);
  }
  exit(0);

  return 0;
}
