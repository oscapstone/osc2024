
#include "header/uart.h"
#include "header/irq.h"
#include "header/shell.h"
#include "header/timer.h"
#include "header/tasklist.h"
#include "header/utils.h"

#define CNTPSIRQ_BIT_POSITION 0x02
#define AUXINIT_BIT_POSTION 1<<29


void except_handler_c() {
	uart_send_string("In Exception handle\n");

	//read spsr_el1
	unsigned long long spsr_el1 = 0;
	asm volatile("mrs %0, spsr_el1":"=r"(spsr_el1));
	uart_send_string("spsr_el1: ");
	uart_hex(spsr_el1);
	uart_send_string("\n");

	//read elr_el1
	unsigned long long elr_el1 = 0;
	asm volatile("mrs %0, elr_el1":"=r"(elr_el1));
	uart_send_string("elr_el1: ");
	uart_hex(elr_el1);
	uart_send_string("\n");
	
	//esr_el1
	unsigned long long esr_el1 = 0;
	asm volatile("mrs %0, esr_el1":"=r"(esr_el1));
	uart_hex(esr_el1);
	uart_send_string("\n");

	//ec
	unsigned ec = (esr_el1 >> 26) & 0x3F; //0x3F = 0b111111(6)
	uart_send_string("ec: ");
	uart_hex(ec);
	uart_send_string("\n");

	
	while(1){

	}
	
}

void timer_irq_handler() {
	//enable core_0_timer
	unsigned int* address = (unsigned int*) CORE0_TIMER_IRQ_CTRL;
	*address = 2;

	asm volatile("msr cntp_ctl_el0,%0"::"r"(0));
	// Disable interrupts to protect critical section
	asm volatile("msr DAIFSet, 0xf");

	uint64_t current_time;
	asm volatile("mrs %0, cntpct_el0":"=r"(current_time));

	while(timer_head && timer_head->expiry <= current_time) {
		timer_t *timer = timer_head;

		//Execute the callback
		timer->callback(timer->data);
 
		// Remove timer from the list
        timer_head = timer->next;
        if (timer_head) {
            timer_head->prev = NULL;
        }
		
		//free timer
		
		// Reprogram the hardware timer if there are still timers left
		if(timer_head) {
			asm volatile("msr cntp_cval_el0, %0"::"r"(timer_head->expiry));
			asm volatile("msr cntp_ctl_el0,%0"::"r"(1));
		} else {
			asm volatile("msr cntp_ctl_el0,%0"::"r"(0));
		}
	

		//enable interrupt
		asm volatile("msr DAIFClr,0xf");
	}

}

void uart_irq_handler(){
	// uart_send_string("in uart_irq_handler\n");
	///uart_hex(uart_write_index);
	///uart_hex(uart_write_head);
	///uart_send_string("\r\n");
	uint32_t iir = mmio_read(AUX_MU_IIR);

    // IF Receive Interrupt
    if ((iir & 0x06) == 0x04) {
        // Read data(8 bytes) and store it in the read buffer
		uart_send_string("in uart_irq_handler_receive\n");
        char data = mmio_read(AUX_MU_IO) & 0xff;
        uart_read_buffer[uart_read_index++] = data;
        if (uart_read_index >= UART_BUFFER_SIZE) {
            uart_read_index = 0;
        }

        // Enqueue the received data into the write buffer for echo
        uart_write_buffer[uart_write_index++] = data;
		// uart_send_string(&data);
        if (uart_write_index >= UART_BUFFER_SIZE) {
            uart_write_index = 0;
        }
		
        // Enable tx interrupt
        mmio_write(AUX_MU_IER, mmio_read(AUX_MU_IER) | 0x2);
    }

    // IF Transmit Interrupt
    if ((iir & 0x06) == 0x02) {
    if (uart_write_head >= UART_BUFFER_SIZE) {
        uart_write_head = 0;
    }
		uart_send_string("in uart_irq_handler_transmit\n");
		// uart_hex(uart_write_index);
		// uart_hex(uart_write_head);
        // Send data from the write buffer
        if (uart_write_head != uart_write_index) {
			uart_send_string("in uart_irq_handler_trans idx!=\n");
			char data = uart_write_buffer[uart_write_head];
			// uart_send_char(data);
			// uart_send_string("\r\n");
			// mmio_write(AUX_MU_IO, 0);
			do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x20));
			*AUX_MU_IO = (unsigned int) data;
			uart_send_string("\r\n");
			uart_write_head++;
            // mmio_write(AUX_MU_IO, uart_write_buffer[uart_write_head++]);
			// uart_write_head ++;
            if (uart_write_index >= UART_BUFFER_SIZE) {
                uart_write_index = 0;
            }
        } 
		else 
		{
			uart_send_string("in uart_irq_handler_trans idx=\n");
			uart_send_string("----------------------------------\n");
            // Disable tx interrupt when there is no data left to send
            mmio_write(AUX_MU_IER, mmio_read(AUX_MU_IER) & ~0x2);
			// mmio_write(AUX_MU_IO, 0);

            /*
			if(uart_read_buffer[uart_read_index-1] == '\r' || uart_read_buffer[uart_read_index-1] == '\n'){
                uart_read_buffer[uart_read_index-1] = '\0';
                uart_read_index = 0;
                uart_write_index = 0;
                uart_write_head = 0;
                // parse_command(uart_read_buffer);
				// uart_write_index = 0;
            }
			*/
        }
    }
	// uart_send_string("out uart_irq_handler\n");	
}


