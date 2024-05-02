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

int main() {
  char str[] = "Hello from EL0.\n";
  /* mini_uart_puts(str); */
  uartwrite(str, sizeof(str));

  char c;
  for (;;) {
    uartread(&c, 1);
    uartwrite(&c, 1);
  }

  return 0;
}
