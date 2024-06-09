#include "mini_uart.h"
#include "shell.h"
#include"stdio.h"
#include"dtb.h"
#include"lock.h"
void kernel_main(void* x0){
	_dtb_addr=x0;
	fdt_traverse(initramfs_callback);
	uart_init();
	irq_vector_init();
	uart_send_string("\r\nthis device is enabled\r\n");
	//uart_irq_on();
	output_daif();
	while (1) {
		shell();
	}
}
