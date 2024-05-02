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

void set32(addr_t address, uint32_t value) {
  asm volatile("str %w[v],[%[a]]" ::[a] "r"(address), [v] "r"(value));
}
uint32_t get32(addr_t address) {
  uint32_t value;
  asm volatile("ldr %w[v],[%[a]]" : [v] "=r"(value) : [a] "r"(address));
  return value;
}

void mini_uart_putc_raw(char c) {
  while ((get32(AUX_MU_LSR_REG) & (1 << 5)) == 0)
    NOP;
  set32(AUX_MU_IO_REG, c);
}

void mini_uart_putc(char c) {
  if (c == '\n')
    mini_uart_putc_raw('\r');
  mini_uart_putc_raw(c);
}

void mini_uart_npf_putc(int c, void* /* ctx */) {
  mini_uart_putc(c);
}

int printf(const char* format, ...) {
  va_list args;
  va_start(args, format);
  int size = npf_vpprintf(&mini_uart_npf_putc, NULL, format, args);
  va_end(args);
  return size;
}

int syscall(int nr) {
  register long x0 asm("x0") = 0;
  register int x8 asm("x8") = nr;
  asm volatile("svc\t0" : "=r"(x0) : "r"(x8));
  return x0;
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

void exit() {
  syscall(5);
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
    exit();
  } else {
    printf("parent here, pid %d, child %d\n", get_pid(), ret);
  }

  exit();
  return 0;
}
