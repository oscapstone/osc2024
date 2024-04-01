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

void set32(addr_t address, uint32_t value) {
  asm volatile("str %w[v],[%[a]]" ::[a] "r"(address), [v] "r"(value));
}
uint32_t get32(addr_t address) {
  uint32_t value;
  asm volatile("ldr %w[v],[%[a]]" : [v] "+r"(value) : [a] "r"(address));
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

void mini_uart_puts(const char* s) {
  for (char c; (c = *s); s++)
    mini_uart_putc(c);
}

int main() {
  mini_uart_puts("Hello from EL0.\n");

  return 0;
}
