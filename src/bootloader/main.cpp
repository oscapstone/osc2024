#include "board/mailbox.hpp"
#include "board/mini-uart.hpp"
#include "board/pm.hpp"

extern char __kernel;

const char usage[] =
    "\n"
    "r\treboot\n"
    "s\tsend kernel\n"
    "j\tjump kernel\n";

void read_kernel() {
  union {
    uint32_t size;
    char buf[4];
  };
  for (int i = 0; i < 4; i++)
    buf[i] = mini_uart_getc();
  mini_uart_printf("Kernel Size: %u\n", size);
  char* kernel = &__kernel;
  for (uint32_t i = 0; i < size; i++)
    kernel[i] = mini_uart_getc();
  mini_uart_printf("Kernel loaded at %p\n", kernel);
}

extern "C" void kernel_main() {
  mini_uart_setup();
  mini_uart_puts("Hello Boot Loader!\n");
  mini_uart_printf("Board revision :\t0x%08X\n", get_board_revision());
  mini_uart_printf("Loaded address :\t%p\n", kernel_main);

  for (;;) {
    char c = mini_uart_getc();
    switch (c) {
      case 'r':
        reboot();
      case 's':
        read_kernel();
        break;
      case 'j': {
        void* addr = &__kernel;
        mini_uart_printf("Jump to kernel @ %p\n", addr);
        wait_cycle(0x10000);
        asm volatile("br %0" : : "r"(addr));
        break;
      }
      default:
        mini_uart_puts(usage);
    }
  }
}
