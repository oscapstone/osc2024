#include "bcm2837/rpi_gpio.h"
#include "bcm2837/rpi_uart1.h"
#include "bcm2837/rpi_irq.h"
#include "uart1.h"
#include "exception.h"
#include "u_string.h"

//implement first in first out buffer with a read index and a write index
char uart_tx_buffer[VSPRINT_MAX_BUF_SIZE]={};
unsigned int uart_tx_buffer_widx = 0;  //write index
unsigned int uart_tx_buffer_ridx = 0;  //read index
char uart_rx_buffer[VSPRINT_MAX_BUF_SIZE]={};
unsigned int uart_rx_buffer_widx = 0;
unsigned int uart_rx_buffer_ridx = 0;

void uart_init()
{
    register unsigned int r;

    /* initialize UART */
    *AUX_ENABLES     |= 1;       // enable UART1
    *AUX_MU_CNTL_REG  = 0;       // disable TX/RX

    /* configure UART */
    *AUX_MU_IER_REG   = 0;       // disable interrupt
    *AUX_MU_LCR_REG   = 3;       // 8 bit data size
    *AUX_MU_MCR_REG   = 0;       // disable flow control
    *AUX_MU_BAUD_REG  = 270;     // 115200 baud rate
    *AUX_MU_IIR_REG   = 0xC6;    // disable FIFO

    /* map UART1 to GPIO pins */
    r = *GPFSEL1;
    r &= ~(7<<12);               // clean gpio14
    r |= 2<<12;                  // set gpio14 to alt5
    r &= ~(7<<15);               // clean gpio15
    r |= 2<<15;                  // set gpio15 to alt5
    *GPFSEL1 = r;

    /* enable pin 14, 15 - ref: Page 101 */
    *GPPUD = 0;
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = (1<<14)|(1<<15);
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = 0;

    *AUX_MU_CNTL_REG = 3;      // enable TX/RX
}

char uart_recv() {
    char r;
    while(!(*AUX_MU_LSR_REG & 0x01)){};
    r = (char)(*AUX_MU_IO_REG);
    uart_send(r);
    if(r =='\r') {uart_send('\r');uart_send('\n');}
    return r=='\r'?'\n':r;
}

void uart_send(char c) {
    while(!(*AUX_MU_LSR_REG & 0x20)){};
    *AUX_MU_IO_REG = c;
}

void uart_2hex(unsigned int d) {
    unsigned int n;
    int c;
    for(c=28;c>=0;c-=4) {
        n=(d>>c)&0xF;
        n+=n>9?0x37:0x30;
        uart_send(n);
    }
}

int  uart_sendline(char* fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    char buf[VSPRINT_MAX_BUF_SIZE];

    char *str = (char*)buf;
    int count = vsprintf(str,fmt,args);

    while(*str) {
        if(*str=='\n')
            uart_send('\r');
        uart_send(*str++);
    }
    __builtin_va_end(args);
    return count;
}

void uart_r_irq_handler(){
    if((uart_rx_buffer_widx + 1) % VSPRINT_MAX_BUF_SIZE == uart_rx_buffer_ridx) // buffer full
    {
        *AUX_MU_IER_REG &= ~(1);  // disable read interrupt
        return; // 退出handler
    }
    // buffer 還沒滿
    uart_rx_buffer[uart_rx_buffer_widx++] = uart_recv(); // 從uart接收字幅，放進buffer
    if(uart_rx_buffer_widx>=VSPRINT_MAX_BUF_SIZE) uart_rx_buffer_widx=0; // widx > size 則重置為0，重複使用buffer
    *AUX_MU_IER_REG |=1; // enable read interrupt 
}

// uart_async_getc read from buffer
// uart_r_irq_handler write to buffer then output
char uart_async_getc() {
    *AUX_MU_IER_REG |=1; // enable read interrupt
    // do whiuart_rx_buffer_ridxle if buffer empty
    while ( uart_rx_buffer_ridx == uart_rx_buffer_widx) *AUX_MU_IER_REG |=1; // enable read interrupt
    // ridx == widx ; 代表buffer為空 ; 再次啟用中斷 ; 確保有新據進入時可以被偵測
    el1_interrupt_disable(); // 讀取數據前，先disable，防止過程又中斷，可能會有race condition
    char r = uart_rx_buffer[uart_rx_buffer_ridx++]; // 讀取字符，並將索引前進移動，準備讀取下一個
    if (uart_rx_buffer_ridx >= VSPRINT_MAX_BUF_SIZE) uart_rx_buffer_ridx = 0; // 若ridx超過範圍，則從開始，循環使用buffer
    el1_interrupt_enable(); // 讀取完，enable
    return r; // 回傳r
}

void uart_w_irq_handler(){
    if(uart_tx_buffer_ridx == uart_tx_buffer_widx) // buffer empty
    {
        *AUX_MU_IER_REG &= ~(2);  // disable write interrupt 沒東西不需要開啟interrupt
        return;  // buffer empty
    }
    // not empty
    uart_send(uart_tx_buffer[uart_tx_buffer_ridx++]); // 發送buffer內的字符並將idx往後一格
    if(uart_tx_buffer_ridx>=VSPRINT_MAX_BUF_SIZE) uart_tx_buffer_ridx=0;
    *AUX_MU_IER_REG |=2;  // enable write interrupt
}

// uart_async_putc writes to buffer
// uart_w_irq_handler read from buffer then output
void uart_async_putc(char c) {
    // if buffer full, wait for uart_w_irq_handler
    while( (uart_tx_buffer_widx + 1) % VSPRINT_MAX_BUF_SIZE == uart_tx_buffer_ridx )  *AUX_MU_IER_REG |=2;  // enable write interrupt 一旦有空間寫入就叫
    el1_interrupt_disable(); // 有空間寫入，開始寫入buffer
    uart_tx_buffer[uart_tx_buffer_widx++] = c; //寫入buffer
    if(uart_tx_buffer_widx >= VSPRINT_MAX_BUF_SIZE) uart_tx_buffer_widx=0;  // cycle pointer
    el1_interrupt_enable(); // 寫完，enable，下次有空間寫入時再叫
    *AUX_MU_IER_REG |=2;  // enable write interrupt
}

int  uart_puts(char* fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    char buf[VSPRINT_MAX_BUF_SIZE];

    char *str = (char*)buf;
    int count = vsprintf(str,fmt,args);

    while(*str) {
        if(*str=='\n')
            uart_async_putc('\r');
        uart_async_putc(*str++);
    }
    __builtin_va_end(args);
    return count;
}


// AUX_MU_IER_REG -> BCM2837-ARM-Peripherals.pdf - Pg.12
void uart_interrupt_enable(){
    *AUX_MU_IER_REG |=1;  // enable read interrupt ; or 0001
    *AUX_MU_IER_REG |=2;  // enable write interrupt ; or 0010
    *ENABLE_IRQS_1  |= 1 << 29;    // Pg.112 29bit設為1 啟用uart中斷
}

void uart_interrupt_disable(){
    *AUX_MU_IER_REG &= ~(1);  // disable read interrupt ; and 1110
    *AUX_MU_IER_REG &= ~(2);  // disable write interrupt ; and 1101
}



