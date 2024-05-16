#include <stdint.h>
typedef volatile char* addr_t;

int main();
void _start() {
  main();
}

int uartread(const char buf[], unsigned size) {
  register uint64_t x0 asm("x0") = (uint64_t)buf;
  register long x1 asm("x1") = size;
  register int x8 asm("x8") = 1;
  asm volatile("svc\t0" : "=r"(x0) : "r"(x0), "r"(x1), "r"(x8));
  return x0;
}

int uartwrite(const char buf[], unsigned size) {
  register uint64_t x0 asm("x0") = (uint64_t)buf;
  register long x1 asm("x1") = size;
  register int x8 asm("x8") = 2;
  asm volatile("svc\t0" : "=r"(x0) : "r"(x0), "r"(x1), "r"(x8));
  return x0;
}

int exec(const char buf[]) {
  register uint64_t x0 asm("x0") = (uint64_t)buf;
  register uint64_t x1 asm("x1") = (uint64_t)0;
  register int x8 asm("x8") = 3;
  asm volatile("svc\t0" : "=r"(x0) : "r"(x0), "r"(x1), "r"(x8));
  return x0;
}

char getc() {
  char c = 0;
  uartread(&c, 1);
  return c;
}

void putc(char c) {
  uartwrite(&c, 1);
}

void exit(int x) {
  register uint64_t x0 asm("x0") = (uint64_t)x;
  register int x8 asm("x8") = 5;
  asm volatile("svc\t0" : "=r"(x0) : "r"(x0), "r"(x8));
}

int main() {
  char str[] = "Hello from EL0.\n";
  /* mini_uart_puts(str); */
  uartwrite(str, sizeof(str));

  for (;;) {
    char c = getc();
    putc(c);
    if (c == 'h')
      exec("hello.img");
    if (c == 'f')
      exec("fork.img");
    if (c == 's')
      exec("syscall.img");
    if (c == 'e')
      exit(0);
  }

  return 0;
}
