
#include "base.h"
#include "uart.h"
#include "peripherals/gpio.h"
#include "peripherals/auxRegs.h"
#include "utils/utils.h"
#include "utils/printf.h"
#include "utils/fifo_buffer.h"
#include "peripherals/irq.h"
#include "proc/worker.h"

static struct FIFO_BUFFER uart_read_fifo;
static char uart_read_buffer[UART_BUFFER_SIZE];

static struct FIFO_BUFFER uart_write_fifo;
static char uart_write_buffer[UART_BUFFER_SIZE];

void uart_init() {

    // initialize the buffer
    fifo_init(&uart_read_fifo, UART_BUFFER_SIZE, uart_read_buffer);
    fifo_init(&uart_write_fifo, UART_BUFFER_SIZE, uart_write_buffer);

    gpio_set_func(TXD, GFAlt5);
    gpio_set_func(RXD, GFAlt5);

    gpio_enable(TXD);
    gpio_enable(RXD);

    utils_delay(500);
    // 1. set AUXENB register to enable mini UART.
    REGS_AUX->enables = 1;
	// 2. Set AUX_MU_CNTL_REG to 0. Disable transmitter and receiver during configuration.
    REGS_AUX->mu_control = 0;

    // bcm pg. 12 
    // enable the receive interrupt, it is in ier 0 bit
    REGS_AUX->mu_ier = 0xfd;

	//// 3. Set AUX_MU_IER_REG to 0. Disable interrupt because currently you don’t need interrupt.
    ////REGS_AUX->mu_ier = 0;
	
    // 4. Set AUX_MU_LCR_REG to 3. Set the data size to 8 bit.
    REGS_AUX->mu_lcr = 3;
	// 5. Set AUX_MU_MCR_REG to 0. Don’t need auto flow control.
    REGS_AUX->mu_mcr = 0;
	// 6. Set AUX_MU_BAUD to 270. Set baud rate to 115200
	// by calculation the BAUD reg should be 270.2673611111 = 270
    REGS_AUX->mu_baud_rate = 270;
	// 7. Set AUX_MU_IIR_REG to 6. No FIFO.
	// 31:8 Reserved, 7:6 FIFO enables, 5:4 zero, 2:1 READ bits WRITE bits
	//	    76543210
	//  0x6 00000110
    REGS_AUX->mu_iir = 0xc6;
	// 8. Set AUX_MU_CNTL_REG to 3. Enable the transmitter and receiver.
    REGS_AUX->mu_control = 3;

    utils_delay(500);
}

void uart_enable_int() {
    REGS_IRQ->irq_enable_1 |= AUX_IRQ;
}

void uart_disable_int() {
    REGS_IRQ->irq_disable_1 |= AUX_IRQ;
}

void uart_send_char(unsigned int c) 
{
    // wait until we can send
    do 
    {
        asm volatile("nop");
    } while (!(REGS_AUX->mu_lsr & 0x20));
    REGS_AUX->mu_io = c;
}

char uart_get_char() 
{
    do
    {
        asm volatile("nop");
    } while (!(REGS_AUX->mu_lsr & 0x01));

    return REGS_AUX->mu_io & 0xff;
}

void uart_send_string(const char* s) 
{
    while (*s) {
		// make new line
		if (*s == '\n')
			uart_send_char('\r');
        uart_send_char(*s++);
    }
}

void uart_send_nstring(unsigned int length, const char* s) 
{
    for (unsigned int i = 0; i < length; ++i) {
        uart_send_char(*s++);
    }
}

/**
 * Display a binary value in hexadecimal
 */
void uart_binary_to_hex(unsigned int d) {
    unsigned int n;
    int c;
    for(c=28;c>=0;c-=4) {
        // get highest tetrad
        n=(d>>c)&0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n+=n>9?0x37:0x30;
        uart_send_char(n);
    }
}

void uart_hex64(U64 value) {
    unsigned int n;
    for(int c = 60; c >= 0;c -= 4) {
        // get highest tetrad
        n=(value >> c)&0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        uart_send_char(n);
    }
}

/**
 * the uart job if interrupt happen
*/
void uart_work() {

    // pg. 13 in the read write bit
    // 10: Receiver holds valid byte
    // so when we and the iir with 4 getting 4
    // mean the receiver holds valid byte
    while ((REGS_AUX->mu_iir & 4) == 4) {
        if (fifo_put(&uart_read_fifo, uart_get_char())) {
            uart_send_string("UART overflow\r\n");
        }
    }


    if ((REGS_AUX->mu_iir & 2) == 2) {
        while (fifo_status(&uart_write_fifo)) {
            uart_send_char(fifo_get(&uart_write_fifo));
        }
        REGS_AUX->mu_ier &= ~(0x2);    // pg. 12 disable the transmit interrupt
    }

    uart_enable_int();
}

// the mini uart interrupt handler
void uart_handle_int() {
    uart_disable_int();
    worker_add(uart_work);
}

void uart_set_transmit_int() {
    REGS_AUX->mu_ier |= 0x2;    // pg. 12 enable the transmit interrupt
}

BOOL uart_async_empty() {
    return (fifo_status(&uart_read_fifo) == 0);
}

char uart_async_get_char() {
    char data = fifo_get(&uart_read_fifo);

    return data;
}

int uart_async_write_char(U8 c) {
    if (fifo_put(&uart_write_fifo, c) == -1) {
        printf("Uart write buffer overflow.\n");
        return -1;
    }
    return 0;
}


