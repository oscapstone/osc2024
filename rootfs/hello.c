#include <stdarg.h>
#include <stdint.h>
typedef volatile char* addr_t;

int main();
void _start() {
  main();
}

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

int exec(const char buf[]) {
  return syscall(3, buf, 0);
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

void exit(int x) {
  syscall(5, x);
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
