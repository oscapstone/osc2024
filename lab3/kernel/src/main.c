
#include "base.h"
#include "io/dtb.h"
#include "io/uart.h"
#include "shell/shell.h"
#include "utils/utils.h"
#include "utils/printf.h"
#include "peripherals/irq.h"
#include "io/exception.h"
#include "peripherals/timer.h"

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

	// init memory management	
	mm_init();

	set_exception_vector_table();
	enable_interrupt_controller();
	irq_enable();

	timer_init();



    shell();

}
