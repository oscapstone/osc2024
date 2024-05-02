#include "mm.h"
#include "gpio.h"
#include "mini_uart.h"
#include "utils.h"

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
void uart_send_uint(unsigned long data) {
    
    // 64-bit
    char str[17];
    int isZero = 1;
    for (int i = 0; i < 16; i++) {
        // Take the LS 4 bits.
        int value = data & 0xF;
        str[15 - i] = int2char(value);
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
    // uart_send_string("\r\n");
}

void uart_send_int(unsigned long data) {
    char str[30];
    int isZero = 1;

    for (int i = 0; i < 30; i++) {
        // Convert to ascii code. '0' <-> 48
        str[30 - 1 - i] = (data % 10) + 48;
        data /= 10;
    }

    for (int i = 0; i < 30; i++) {
        if (str[i] != '0') {
            isZero = 0;
        }

        if (!isZero) {
            uart_send(str[i]);
        }
    }

    if (isZero) {
        uart_send('0');
    }

}

void enable_uart_interrupt(void) {
    /*
     *  Manual p.12
     *  If AUX_MU_IER_REG[0] is set, receive interrupts are enabled.
     *  If AUX_MU_IER_REG[1] is set, transmit interrupts are enabled.
     */ 
    
    *AUX_MU_IER_REG |= 3u;

    // Enable second level interrupt controller.
    volatile unsigned int* IRQ_ENABLE_S1 = (volatile unsigned int *)(PBASE + 0x0000B210);
    *IRQ_ENABLE_S1 |= (1 << 29);
}

void disable_uart_interrupt(void) {

    // Disable both receive and transmit interrupts.
    *AUX_MU_IER_REG = 0u;
}

// #define ASYNC_BUF_SIZE 256
// volatile char rx_buf_async[ASYNC_BUF_SIZE];
// volatile int rx_buf_head = 0;
// volatile int rx_buf_tail = 0;

// volatile char tx_buf_async[ASYNC_BUF_SIZE];
// volatile int tx_buf_head = 0;
// volatile int tx_buf_tail = 0;

// void handle_uart_interrupt() {

//     // disable_uart_interrupt();

//     // On manual P.13
//     // Receive interrupt.(Receiver holds valid byte)
//     if (*AUX_MU_IIR_REG & 0x4) {

//         // Read the least 8-bits(1 byte, the length of char) in IO register.
//         // Indicating data is sent from laptop.
//         char c = *AUX_MU_IO_REG & 0xFF;
//         int next = (rx_buf_tail + 1) % ASYNC_BUF_SIZE;

//         // Check buffer overflow.
//         if (next != rx_buf_head) {
//             rx_buf_async[rx_buf_tail] = c;
//             rx_buf_tail = next;
//         }
//     }
// /*
//     // Transmit interrupt.(Transmit register empty)
//     else if ((*AUX_MU_IIR_REG & 0x2) && (tx_buf_head != tx_buf_tail)) {
//         *AUX_MU_IO_REG = tx_buf_async[tx_buf_head];
//         tx_buf_head = (tx_buf_head + 1) % ASYNC_BUF_SIZE;
//     }
// */

//     // enable_uart_interrupt();
    
// }

// int uart_recv_async(char* buf, int buf_size) {
//     int bytes_read = 0;

//     // Write the data within the RX buffer into buf.
//     // Continue writing until the RX buffer is empty or exceed buf_size.
//     while ((rx_buf_head != rx_buf_tail) && (bytes_read < buf_size)) {
//         buf[bytes_read++] = rx_buf_async[rx_buf_head];
//         rx_buf_head = (rx_buf_head + 1) % ASYNC_BUF_SIZE;
//     }

//     return bytes_read;
// }

// void uart_send_async(const char* data, int len) {
//     for (int i = 0; i < len; i++) {
//         int next = (tx_buf_tail + 1) % ASYNC_BUF_SIZE;

//         // Wait until Transmit buffer isn't full.
//         if (next == tx_buf_head) {
//             continue;
//         }

//         tx_buf_async[tx_buf_tail] = data[i];
//         tx_buf_tail = next;
//     }

//     if (tx_buf_head == tx_buf_tail) {
//         // Disable TX interrupt if no data
//         *AUX_MU_IER_REG &= ~0x2;
//     } else {
//         *AUX_MU_IER_REG |= 0x2;
//     }
// }

#define BUFFER_SIZE 256
volatile char tx_buffer[BUFFER_SIZE];
volatile unsigned int tx_head = 0;
volatile unsigned int tx_tail = 0;

// UART interrupt handler
void handle_uart_interrupt(void* data) {
    // Check if the interrupt is for a received character
    if (*AUX_MU_IIR_REG & 0x4) { // Receiver holds a valid byte
        char received_char = *AUX_MU_IO_REG & 0xFF; // Read the received byte
        uart_send_async(received_char); // Echo it back immediately or queue
    }

    // Transmit buffer management
    if ((*AUX_MU_IIR_REG & 0x2) && (tx_head != tx_tail)) { // Transmit register empty
        *AUX_MU_IO_REG = tx_buffer[tx_tail]; // Send next character
        tx_tail = (tx_tail + 1) % BUFFER_SIZE; // Move tail forward
    }
}

// Queue a character for transmission or send immediately if ready
void uart_send_async(char c) {
    // Attempt immediate transmission if UART is ready and buffer is empty
    if ((*AUX_MU_LSR_REG & 0x20) && (tx_head == tx_tail)) {
        *AUX_MU_IO_REG = c; // Transmit immediately
    } else {
        // Otherwise, queue the character
        unsigned int next_head = (tx_head + 1) % BUFFER_SIZE;
        if (next_head != tx_tail) { // Ensure the buffer isn't full
            tx_buffer[tx_head] = c;
            tx_head = next_head;
            *AUX_MU_IER_REG |= 0x2; // Ensure transmit interrupts are enabled
        }
    }
}