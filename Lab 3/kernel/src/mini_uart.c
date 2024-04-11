#include "type.h"
#include "gpio.h"
#include "aux.h"
#include "delay.h"
#include "irq.h"


/**
 * Set baud rate and characteristics (115200 8N1) and map to GPIO
 */
void 
mini_uart_init(void)
{
    /*
        setup GPIO pins
    */

    uint32_t selector;

    selector            =   *GPFSEL1;
    selector            &=  ~((7<<12)|(7<<15)); // clean gpio14 and gpio15
    selector            |=  (2<<12)|(2<<15);    // set alt5 for gpio14 and gpio15
    *GPFSEL1            =   selector;

    *GPPUD              =   0;
    
    delay_cycles(150);
    
    *GPPUDCLK0          =   (1<<14)|(1<<15);    // enable pins 14 and 15
    
    delay_cycles(150);
    
    *GPPUDCLK0          =   0;                  // flush GPIO setup

    /* 
        setup AUX mini UART 
    */

    *AUX_ENABLES        |=  1;                  // Enable UART1, AUX mini uart (this also enables access to its registers)
    *AUX_MU_CNTL        =   0;                  // Disable auto flow control and disable receiver and transmitter (for now)

    *AUX_MU_LCR         =   3;                  // Enable 8 bit mode
    *AUX_MU_MCR         =   0;                  // Set RTS line to be always high

    *AUX_MU_IER         =   0;                  // Disable receive and transmit interrupts

    *AUX_MU_IIR         =   0xc6;               // disable interrupts
    *AUX_MU_BAUD        =   270;                // Set baud rate to 115200

    *AUX_MU_CNTL        =   3;                  // Finally, enable transmitter and receiver
}


/**
 * Receive a character
 * ref : BCM2837-ARM-Peripherals p5
 * AUX_MU_LSR_REG bit_0 == 1 -> readable
 */
uint8_t 
mini_uart_getc() 
{
    /* wait until something is in the buffer */
    do { asm volatile("nop"); } while (!(*AUX_MU_LSR & 0x01));
    /* read it and return */
    uint8_t r = (uint8_t)(*AUX_MU_IO & 0xFF);
    /* convert carriage return to newline */
    return r == '\r' ? '\n' : r;
}

uint8_t 
mini_uart_getb() 
{
    /* wait until something is in the buffer */
    do { asm volatile("nop"); } while (!(*AUX_MU_LSR & 0x01));
    /* read it and return */
    return (uint8_t)(*AUX_MU_IO | 0xFF);
}


/**
 * Send a character
 * ref : BCM2837-ARM-Peripherals p5
 * AUX_MU_LSR_REG bit_5 == 1 -> writable
 */
void mini_uart_putc(uint8_t c) 
{
    /* wait until we can send */
    do { asm volatile("nop"); } while (!(*AUX_MU_LSR & 0x20));
    /* write the character to the buffer */
    *AUX_MU_IO = c;
}


/**
 * Display a string
 */
void 
mini_uart_puts(byteptr_t s) 
{
    while (*s) {
        mini_uart_putc(*s++);
    }
}

/**
 * Display a string with the newline
 */
void 
mini_uart_putln(byteptr_t s) 
{
    mini_uart_puts(s);
    mini_uart_puts("\r\n");
}


/**
 * Display a 32-bit binary value in hexadecimal
 */
void 
mini_uart_hex(uint32_t d)
{   
    for (int32_t c = 28; c >= 0; c -= 4) {
        // get highest tetrad
        uint8_t n = (d >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        mini_uart_putc(n);
    }
}

/**
 * Display a 64-bit binary value in hexadecimal
 */
void 
mini_uart_hexl(uint64_t d)
{
    for (int32_t c = 60; c >= 0; c -= 4) {
        // get highest tetrad
        uint8_t n = (d >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        mini_uart_putc(n);
    }
}


void 
mini_uart_endl()
{
    mini_uart_puts("\r\n");
}



#define BUFFER_SIZE 0x100

static uint8_t _read_buffer[BUFFER_SIZE];
static uint8_t _write_buffer[BUFFER_SIZE];

static uint32_t _read_start, _read_end;
static uint32_t _write_start, _write_end;


byte_t
mini_uart_async_getc()
{
    aux_set_rx_interrupt();
    while (_read_start == _read_end) {
        asm volatile("nop");
    }
    uint8_t c = _read_buffer[_read_start++];
    _read_start &= 0xFF;
    return c;
}

void
mini_uart_async_putc(uint8_t c)
{
    _write_buffer[_write_end++] = c;
    _read_end &= 0xFF;
    aux_set_tx_interrupt();
}

void
mini_uart_async_puts(byteptr_t s)
{   
    while (*s) {
        _write_buffer[_write_end++] = *s++;
        _write_end &= 0xFF;
    }
    aux_set_tx_interrupt();
}


void
mini_uart_interrupt_handler(byteptr_t arg)
{
    irq_disable_aux_interrupt();

    uint32_t tx = *AUX_MU_IIR & 0x2;
    uint32_t rx = *AUX_MU_IIR & 0x4;

    uint32ptr_t ier = (uint32ptr_t) arg;

    if (rx) {
        uint8_t c = mini_uart_getc();
        _read_buffer[_read_end++] = c;
        _read_end &= 0xFF;
    }
    else if (tx) {
        if (_write_start == _write_end) {
            *ier &= ~(0x2);
        } else {
            uint8_t c = _write_buffer[_write_start++];
            _write_start &= 0xFF;
            mini_uart_putc(c);
        }
    }
    *AUX_MU_IER |= *ier;
    irq_enable_aux_interrupt();
}


void
mini_uart_handler()
{
    uint32_t ier = *AUX_MU_IER; 
    irq_disable_aux_interrupt();

    uint32_t tx = *AUX_MU_IIR & 0x2;
    uint32_t rx = *AUX_MU_IIR & 0x4;

    if (rx) {
        uint8_t c = mini_uart_getc();
        _read_buffer[_read_end++] = c;
        _read_end &= 0xFF;
    }
    else if (tx) {
        if (_write_start == _write_end) {
            ier &= ~(0x2);
        } else {
            uint8_t c = _write_buffer[_write_start++];
            _write_start &= 0xFF;
            mini_uart_putc(c);
        }
    }
    *AUX_MU_IER = ier;
    irq_enable_aux_interrupt();
}

void 
mini_uart_async_demo()
{
    irq_enable_aux_interrupt();

    uint8_t c;
    do {
        c = mini_uart_async_getc();
        _write_buffer[_write_end++] = c;
        _write_end &= 0xFF;
    } while (c != '\n');
    _write_buffer[_write_end] = '\0';
    aux_set_tx_interrupt();

    irq_disable_aux_interrupt();
}