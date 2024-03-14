#include "include/mm.h"
#include "include/peripherals/gpio.h"
#include "include/peripherals/mini_uart.h"
#include "utils.h"

void delay(unsigned int clock)
{
    while (clock--) {
        // Each "nop" instruction consumes one clock cycle.
        asm volatile("nop");
    }
}

void uart_init (void)
{
    unsigned int selector;

    // Set the bits in register GPFSEL1 for gpio14 and gpio15, configuring them
    // for the use of mini UART.
    selector = *GPFSEL1;
    selector &= ~(7u << 12);                   // clean gpio14
    selector |= 2u << 12;                      // set alt5 for gpio14
    selector &= ~(7u << 15);                   // clean gpio15
    selector |= 2u << 15;                      // set alt5 for gpio 15
    *GPFSEL1 = selector;

    // Switching the states of the pin.
    // Write to GPPUD to set the required control signal (i.e. Pull-up or Pull-Down or neither
    // to remove the current Pull-up/down).
    *GPPUD = 0u;

    // Wait 150 cycles – this provides the required set-up time for the control signal.
    // delay(150);
    delay(150u);

    // Write to GPPUDCLK0/1 to clock the control signal into the GPIO pads you wish to 
    // modify – NOTE only the pads which receive a clock will be modified, all others 
    // will retain their previous state.
    *GPPUDCLK0 = (1u << 14) | (1u << 15);
    delay(150);

    // Write to GPPUDCLK0/1 to remove the clock
    *GPPUDCLK0 = 0u;

    // Enabling mini uart
    *AUX_ENABLES = 1u;          // Enable mini uart (this also enables access to its registers)
    *AUX_MU_CNTL_REG = 0u;      // Disable auto flow control and disable receiver and transmitter (for now)
    *AUX_MU_IER_REG = 0u;       // Disable receive and transmit interrupts
    *AUX_MU_LCR_REG = 3u;       // Enable 8 bit mode
    *AUX_MU_MCR_REG = 0u;       // Set RTS line to be always high
    *AUX_MU_BAUD_REG = 270u;    // Set baud rate to 115200(meaning the serial port is capable of transferring
                                // a maximum of 115200 bits per second)
                                // baudrate = (system_clk_freq = 250MHz) / (8 * (baudrate_reg + 1))
    *AUX_MU_IIR_REG = 6u;
    *AUX_MU_CNTL_REG = 3u;      //Finally, enable transmitter and receiver
}

void uart_send (char c)
{
    // bit_5 == 1 -> writable
    // 0x20 = 0000 0000 0010 0000
    while (!((*AUX_MU_LSR_REG) & 0x20));
    *AUX_MU_IO_REG = c;
}

char uart_recv (void)
{
    // bit_0 == 1 -> readable
    // 0x01 = 0000 0000 0000 0001
    while (!((*AUX_MU_LSR_REG) & 0x01));

    return (*AUX_MU_IO_REG & 0xFF);
}

void uart_send_string(char* str)
{
	for (int i = 0; str[i] != '\0'; i++) {
		uart_send((char)str[i]);
	}
}

// Convert unsigned int to string and send it to uart.
void uart_send_uint(unsigned int data) {
    
    // 64-bit
    char str[17];
    int isZero = 1;
    for (int i = 0; i < 16; i++) {
        // Take the LS 4 bits.
        int value = data & 0xF;
        str[15 - i] = int2str(value);
        data = data >> 4;
    }

    for (int i = 0; i < 16; i++) {
        // Don't print the preceding zeros.
        if (str[i] != '0') {
            isZero = 0;
        }

        if (!isZero) {
            uart_send(str[i]);
        }
    }

    // If the value is zero.
    if (isZero)
        uart_send('0');
    uart_send_string("\r\n");
}