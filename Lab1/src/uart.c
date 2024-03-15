#include "aux.h"
#include "gpio.h"
#include "mystring.h"

void uart_init() {
    /* Initialize UART */
    *AUX_ENABLES |= 1;   // Enable mini UART
    *AUX_MU_CNTL = 0;    // Disable TX, RX during configuration
    *AUX_MU_IER = 0;     // Disable interrupt
    *AUX_MU_LCR = 3;     // Set the data size to 8 bit
    *AUX_MU_MCR = 0;     // Don't need auto flow control
    *AUX_MU_BAUD = 270;  // Set baud rate to 115200
    *AUX_MU_IIR = 0xc6;     // No FIFO

    /* Map UART to GPIO Pins */

    // 1. Change GPIO 14, 15 to alternate function
    register unsigned int r = *GPFSEL1;
    // r &= ~(7<<12);                   // clean gpio14
    // r |= 2<<12;                      // set alt5 for gpio14
    // r &= ~(7<<15);                   // clean gpio15
    // r |= 2<<15;                      // set alt5 for gpio 15
    r&=~((7<<12)|(7<<15)); // gpio14, gpio15
    r|=(2<<12)|(2<<15);    // alt5
    *GPFSEL1 = r;

    // 2. Disable GPIO pull up/down (Because these GPIO pins use alternate functions, not basic input-output)
    // Set control signal to disable
    *GPPUD = 0;
    // Wait 150 cycles
    r = 150;
    while (r--) {
        asm volatile("nop");
    }
    // Clock the control signal into the GPIO pads
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    // Wait 150 cycles
    r = 150;
    while (r--) {
        asm volatile("nop");
    }
    // Remove the clock
    *GPPUDCLK0 = 0;

    // 3. Enable TX, RX
    *AUX_MU_CNTL = 3;
}

char uart_recv() {
    // Check data ready field
    do {
        asm volatile("nop");
    } while (!(*AUX_MU_LSR & 0x01));
    // Read
    char r = (char)(*AUX_MU_IO);
    // Convert carrige return to newline
    return r == '\r' ? '\n' : r;
}

void uart_send(unsigned int c) {
    // Check transmitter idle field
    do {
        asm volatile("nop");
    } while (!(*AUX_MU_LSR & 0x20));
    // send
    *AUX_MU_IO = c;
}
void uart_printf(char* fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    extern volatile unsigned char _end;  // defined in linker
    char* s = (char*)&_end;              // put temporary string after code
    //
    //vsprintf(s, fmt);//, args);
    //
    //unsigned int vsprintf(char *dst, char *fmt){//, __builtin_va_list args){
    
    char *dst_orig = s;
    char *dst = s;
    while (*fmt) {
        
        if (*fmt == '%') {
            fmt++;
            // escape %
            if (*fmt == '%') {
                goto put;
            }
            // string
            if (*fmt == 's') {
                char *p = __builtin_va_arg(args, char *);
                while (*p) {
                    *dst++ = *p++;
                }
            }
            // number
            if (*fmt == 'd') {
                int arg = __builtin_va_arg(args, int);
                char buf[11];
                char *p = itoa(arg, buf);
                while (*p) {
                    *dst++ = *p++;
                }
            }
            // float
            if (*fmt == 'f') {
                float arg = (float) __builtin_va_arg(args, double);
                char buf[19];  // sign + 10 int + dot + 6 float
                char *p = ftoa(arg, buf);
                while (*p) {
                    *dst++ = *p++;
                }
            }
            // hex
            if (*fmt == 'x') {
                // get %x's value
                int arg = __builtin_va_arg(args, int);
                char buf[8 + 1];
                char *p = itox(arg, buf);
                while (*p) {
                    *dst++ = *p++;
                }
            }
        } else {
        put:
            *dst++ = *fmt;
            
        }
        fmt++;
       
    }
    *dst = '\0';
    //
    while (*s) {
        if (*s == '\n') uart_send('\r');
        uart_send(*s++);
    }
}

void uart_flush() {
    while (*AUX_MU_LSR & 0x01) {
        *AUX_MU_IO;
    }
}


void uart_puts(char *s) {
    while(*s) {
        /* convert newline to carriage return + newline */
        if(*s=='\n')
            uart_send('\r');
        uart_send(*s++);
    }
}

