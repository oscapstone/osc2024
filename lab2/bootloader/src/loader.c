#include "mini_uart.h"
#include "loader.h"

extern char _kernel[];

typedef void (*kernel_funcp)(char*);

void load_kernel(char* fdt)
{
    // Loading kernel Protocol:
    //  4 bytes: The kernel image length n
    //  n bytes: The kernel image
    // Bootloader will store kernel to 0x80000 and then jump to it
	unsigned int len;
	char *p = _kernel;

	uart_printf("[*] Kernel base address: %x\r\n", _kernel);

    len = uart_recv_uint();

    uart_printf("[*] Kernel image length: %d\r\n", len);

    while (len--) {
        *p++ = uart_recv();
    }

	// Execute kernel
    ((kernel_funcp)_kernel)(fdt);
}
