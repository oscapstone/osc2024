#include "uart.h"


char uart_tx_buffer[MAX_BUF_SIZE] = {};
char uart_rx_buffer[MAX_BUF_SIZE] = {};


static unsigned int uart_tx_buffer_r_idx;
static unsigned int uart_tx_buffer_w_idx;
static unsigned int uart_rx_buffer_r_idx;
static unsigned int uart_rx_buffer_w_idx;

/**
 * Set baud rate and characteristics (115200 8N1) and map to GPIO
 */
void init_idx() {
    uart_tx_buffer_r_idx = 0;
    uart_tx_buffer_w_idx = 0;
    uart_rx_buffer_r_idx = 0;
    uart_rx_buffer_w_idx = 0;
}
void uart_init()
{
    register unsigned int r;
    
    r = *GPFSEL1;
    //gpio15 can be both used for mini UART and PL011 UART
    r &= ~((7<<12)|(7<<15)); // gpio14 least bit is 12, gpio15 least bit is 15
    r |= (2<<12)|(2<<15);    // set alt5 for gpio14 and gpio15
    *GPFSEL1 = r;          // control gpio pin 10~19
    *GPPUD = 0;            // enable pins 14 and 15
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = (1<<14)|(1<<15);
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = 0;        // flush GPIO setup
    
    /* initialize UART */
    *AUX_ENABLE |= 1;      // enable UART1, AUX mini uart
    *AUX_MU_CNTL = 0;      // disable tx,rx during configuration
    *AUX_MU_IER = 0;       // disable tx/rx interrupts
    *AUX_MU_LCR = 3;       // set data size 8 bits
    *AUX_MU_MCR = 0;       // don't need auto flow control
    *AUX_MU_BAUD = 270;    // 115200 baud, system clock 250MHz
    *AUX_MU_IIR = 0x6;     // clear FIFO
    *AUX_MU_CNTL = 3;      // enable Tx, Rx
    
    /*
    for(int i = 0; i < MAX_BUF_SIZE; ++i) {
        uart_tx_buffer[i] = '\0';
        uart_rx_buffer[i] = '\0';
    }*/
}

/**
 * Send a character
 */
void uart_send(unsigned int c) {
    /* wait until we can send */
    do{
        asm volatile("nop");
    }while(!(*AUX_MU_LSR & 0x20));
    /* write the character to the buffer */
    *AUX_MU_IO = (unsigned int)c;
}


char uart_getc() {
    char r;
    /* wait until something is in the buffer */
    //uart_puts("getc\n");
    do{
        asm volatile("nop");
    }while(!(*AUX_MU_LSR & 0x01));
    //uart_puts("read\n");
    /* read it and return */
    r = (char)(*AUX_MU_IO);
    /* convert carriage return to newline */
    return r=='\r'?'\n':r;
}


void uart_puts(char *s) {
    while(*s) {
        /* convert newline to carriage return + newline */
        if(*s=='\n')
            uart_send('\r');
        uart_send(*s++);
    }
}

void uart_hex(unsigned int d) {
    unsigned int n;
    int c;
    for(c=28;c>=0;c-=4) {
        // get highest tetrad
        n=(d>>c)&0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n+=n>9?0x37:0x30;
        uart_send(n);
    }
}

void uart_dec(int d) {
    if (d < 0) {
        uart_send('-');
        d = -d;
    }
    int divisor = 1;
    while (divisor <= d / 10) {
        divisor *= 10;
    }
    while (divisor > 0) {
        uart_send((d / divisor) + '0');
        d %= divisor;
        divisor /= 10;
    }
}

extern char _end;

int uart_printf(char *fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    char buf[MAX_BUF_SIZE];
    char *s = (char *)buf;
    int count = vsprintf(s, fmt, args);
    while (*s)
    {
        if (*s == '\n')
            uart_send('\r');
        uart_send(*s++);
    }
    __builtin_va_end(args);
    return count;
}

void uart_async_putc(char c) {
    // is full
    // send a byte to transmit fifo
    while((uart_tx_buffer_w_idx + 1) % MAX_BUF_SIZE == uart_tx_buffer_r_idx)
        enable_mini_uart_tx_interrupt();

    disable_interrupt();

    // put a byte to tx buffer
    //if(c == '\n')
      //  uart_tx_buffer[uart_tx_buffer_w_idx++] = '\r';
    uart_tx_buffer[uart_tx_buffer_w_idx++] = c;
    uart_tx_buffer_w_idx %= MAX_BUF_SIZE;

    enable_interrupt();
    
    enable_mini_uart_tx_interrupt();
}

char uart_async_getc() {
    // is empty
    // if empty rx buffer get a byte from IO
    //uart_puts("call rx\n");
    // fix test
    enable_mini_uart_rx_interrupt();
    while((uart_rx_buffer_w_idx == uart_rx_buffer_r_idx))
        enable_mini_uart_rx_interrupt();
    
    //uart_puts("agetc\n");
    disable_interrupt();
    // get a byte from rx buffer
    char c = uart_rx_buffer[uart_rx_buffer_r_idx++];
    uart_rx_buffer_r_idx %= MAX_BUF_SIZE;

    enable_interrupt();
    

    return c;
}

// char sync uart to async
int uart_async_printf(char *fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    char buf[MAX_BUF_SIZE];
    char *s = (char *)buf;
    int count = vsprintf(s, fmt, args);
    while (*s)
    {
        if (*s == '\n')
            uart_async_putc('\r');
        uart_async_putc(*s++);
    }
    __builtin_va_end(args);
    return count;
}

void enable_mini_uart_interrupt() {
    //uart_puts("a1\n");
    enable_mini_uart_rx_interrupt();
    //uart_puts("a2\n");
    enable_mini_uart_tx_interrupt();

    // second level interrupt controller
    // might be block
    //uart_puts("a5\n");
    *ENABLE_IRQS_1 |= 1 << 29;
    uart_puts("a4\n");
}

// rx interrupt bit 1
void enable_mini_uart_rx_interrupt() {
    *AUX_MU_IER |= 1;
}
// clear bit 1
void disable_mini_uart_rx_interrupt(){
    // 11111...10
    *AUX_MU_IER &= ~1;
}

// tx interrupt bit 2
void enable_mini_uart_tx_interrupt() {
    *AUX_MU_IER |= 1 << 1;
}
// clear bit 2
void disable_mini_uart_tx_interrupt(){
    // 111111...01
    *AUX_MU_IER &= ~(1<<1);
}

void unmask_aux_interrupt(){
    *ENABLE_IRQS_1 |= IRQ_PENDING_1_AUX_INT;
}

void mask_aux_int(){
    *ENABLE_IRQS_1 &= ~IRQ_PENDING_1_AUX_INT;
}

void uart_tx_interrupt_handler() {
    // check empty
    // empty then return 
    if(uart_tx_buffer_r_idx == uart_tx_buffer_w_idx) {
        disable_mini_uart_tx_interrupt();
        return;
    }

    // sent a byte to transmit fifo
    disable_interrupt();
    // *AUX_MU_IO = (unsigned int)uart_tx_buffer[uart_tx_buffer_r_idx++];
    uart_send(uart_tx_buffer[uart_tx_buffer_r_idx++]);
    uart_tx_buffer_r_idx %= MAX_BUF_SIZE;
    enable_interrupt();

    enable_mini_uart_tx_interrupt();
}

void uart_rx_interrupt_handler() {
    // is holding byte
    //uart_puts("rx handler 111\n");
    if((uart_rx_buffer_w_idx + 1) % MAX_BUF_SIZE == uart_rx_buffer_r_idx) {
        disable_mini_uart_rx_interrupt();
        return;
    }

    // get a byte from receive fifo
    //uart_puts("rx handler\n");
    disable_interrupt();
    uart_rx_buffer[uart_rx_buffer_w_idx++] = (char)(*AUX_MU_IO);
    //uart_send(uart_rx_buffer[uart_rx_buffer_w_idx++])
    uart_rx_buffer_w_idx %= MAX_BUF_SIZE;
    enable_interrupt();

    // enable_mini_uart_rx_interrupt();
}


