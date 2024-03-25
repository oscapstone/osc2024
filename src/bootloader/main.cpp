#include "board/mailbox.hpp"
#include "board/mini-uart.hpp"
#include "board/pm.hpp"

extern char __kernel[];
char* kernel_addr = __kernel;

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
    buf[i] = mini_uart_getc_raw();
  mini_uart_printf("Kernel Size: %u\n", size);
  for (uint32_t i = 0; i < size; i++)
    kernel_addr[i] = mini_uart_getc_raw();
  mini_uart_printf("Kernel loaded @ %p\n", kernel_addr);
}

extern "C" void kernel_main(void* dtb_addr) {
  mini_uart_setup();
  mini_uart_puts("Hello Boot Loader!\n");
  mini_uart_printf("Board revision :\t0x%08X\n", get_board_revision());
  mini_uart_printf("Loaded address :\t%p\n", kernel_main);
  mini_uart_printf("Kernel address :\t%p\n", kernel_addr);
  mini_uart_printf("DTB address    :\t%p\n", dtb_addr);

  for (;;) {
    char c = mini_uart_getc();
    switch (c) {
      case 'r':
        reboot();
      case 's':
        read_kernel();
        break;
      case 'j': {
        mini_uart_printf("Jump to kernel @ %p\n", kernel_addr);
        wait_cycle(0x1000);
        (decltype (&kernel_main)(kernel_addr))(dtb_addr);
        break;
      }
      default:
        mini_uart_puts(usage);
    }
  }
}
