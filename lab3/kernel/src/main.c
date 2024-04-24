
#include "base.h"
#include "io/dtb.h"
#include "io/uart.h"
#include "shell/shell.h"
#include "utils/utils.h"
#include "utils/printf.h"
#include "peripherals/irq.h"
#include "io/exception.h"

void putc(void *p, char c) {
	if (c == '\n') {
		uart_send_char('\r');
	}
	uart_send_char(c);
}

void main() {

    // initialze UART
    uart_init();

	init_printf(0, putc);

	set_exception_vector_table();
	enable_interrupt_controller();
	irq_enable();

	// TODO: bugge here 
	//fdt_traverse(get_cpio_addr);

	// while (1) {
	// 	asm volatile("nop");
	// }

    shell();

}
