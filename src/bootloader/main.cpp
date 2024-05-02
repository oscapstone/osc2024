#include "main.hpp"

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
  mini_uart_printf("[b] Kernel Size: %u\n", size);
  for (uint32_t i = 0; i < size; i++)
    kernel_addr[i] = mini_uart_getc_raw();
  mini_uart_printf("[b] Kernel loaded @ %p\n", kernel_addr);
}

void bootloader_main(void* dtb_addr) {
  mini_uart_setup();
  mini_uart_puts("[b] Hello Boot Loader!\n");
  mini_uart_printf("[b] Board revision :\t0x%08X\n", get_board_revision());
  mini_uart_printf("[b] Loaded address :\t%p\n", bootloader_main);
  mini_uart_printf("[b] Kernel address :\t%p\n", kernel_addr);
  mini_uart_printf("[b] DTB address    :\t%p\n", dtb_addr);

  for (;;) {
    char c = mini_uart_getc();
    switch (c) {
      case 'r':
        reboot();
      case 's':
        read_kernel();
        break;
      case 'j': {
        mini_uart_printf("[b] Jump to kernel @ %p\n", kernel_addr);
        delay(0x1000);
        (decltype (&kernel_main)(kernel_addr))(dtb_addr);
        break;
      }
      default:
        mini_uart_puts(usage);
    }
  }
}
