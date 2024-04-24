
#include "uart.h"
#include "peripherals/gpio.h"
#include "peripherals/aux.h"
#include "utils/utils.h"

#define TXD 14
#define RXD 15

void uart_init() {

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

void uart_printU32(U32 value) {
    unsigned char s[10];
    U32 i = 0;
    uart_send_string("coverting int ...\r\n");
    do {
        uart_send_string("test\r\n");
        char remainder = (char)(value % 10);
        uart_send_string("test\r\n");
        s[i++] = (char)(remainder + 0x37);
        uart_send_string("test\r\n");
        value = value / 10;
        if (i == 10)
            break;
        uart_send_string("test\r\n");
    } while (value != 0);
    uart_send_string("printing int ...\r\n");
    for (U32 j = 0; j < i ;j++) {
        uart_send_char(s[i - j]);
    }
}



