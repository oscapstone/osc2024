#include "uart.h"
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include "interrupt.h"
#include "utils.h"
#include "except_c.h"
#include "shell.h"
#include "printf.h"
#include "time_c.h"
#include "tasklist.h"

#define RX_INTERRUPT_BIT    0x01
#define TX_INTERRUPT_BIT    0x02

#define AUXINIT_BIT_POSTION 1<<29

#define BUFFER_MAX_SIZE 256u

// Create a Uart read buffer
char read_buf[BUFFER_MAX_SIZE];
// Create a Uart write buffer
char write_buf[BUFFER_MAX_SIZE];

char empty_buf[BUFFER_MAX_SIZE];

// Buffer state
int read_buf_start, read_buf_end;
int write_buf_start, write_buf_end;

void uart_buf_init(){
    for(int i = 0; i < BUFFER_MAX_SIZE; i++){
        write_buf[i] = empty_buf[i];
        read_buf[i] = empty_buf[i];
    }
    read_buf_start = read_buf_end = 0;
    write_buf_start = write_buf_end = 0;
}

void delay(unsigned int clock)
{
    while (clock--)
    {
        asm volatile("nop");
    }
}

void uart_init(){
    register unsigned int r;

    r=*GPFSEL1;
    r&=~((7<<12)|(7<<15)); // gpio14, gpio15 clear to 0
    r|=(2<<12)|(2<<15);    // set gpio14 and 15 to 010/010 which is alt5
    *GPFSEL1 = r;          // from here activate Trasmitter&Receiver

    *GPPUD = 0;
    r=150; while(r--){ asm volatile("nop"); } //delay(150)
    *GPPUDCLK0 = (1<<14)|(1<<15);
    // GPIO control 54 pins
    // GPPUDCLK0 controls 0-31 pins
    // GPPUDCLK1 controls 32-53 pins
    // set 14,15 bits = 1 which means we will modify these two bits
    // trigger: set pins to 1 and wait for one clock

    r=150; while(r--){ asm volatile("nop"); } //delay(150)
    *GPPUDCLK0 = 0;        // flush GPIO setup
    r=500; while(r--){ asm volatile("nop"); } //delay(500)

    /* initialize UART */
    *AUX_ENABLE |= 1;   //Enable mini uart
    *AUX_MU_CNTL_REG = 0;   //Disable auto flow control, reciver, and transmitter
    *AUX_MU_IER_REG = 0;    //Disable receive and transmit interrupts
    *AUX_MU_LCR_REG = 3;    //Enalbe 8 bit mode
    *AUX_MU_MCR_REG = 0;    //Set RTS line to be always high
    *AUX_MU_BAUD_REG = 270; //Set baud rate to 115200
    *AUX_MU_IIR_REG = 0x6;  //0xc6 = 000110 
                            //bit 6 bit 7 No FIFO. Sacrifice reliability(buffer) to get low latency 
                            //Writing with bit 1 set will clear the receive FIFO
                            //Writing with bit 2 set will clear the transmit FIFO
    *AUX_MU_CNTL_REG = 3;   //enable transmitter and receiver

    read_buf_start = read_buf_end = 0;
    write_buf_start = write_buf_end = 0;
}

void uart_send_char(unsigned int c){
    while(!(*AUX_MU_LSR_REG&0x20));
    *AUX_MU_IO_REG=c;
}

char uart_get_char(){
    char r;
    while(!(*AUX_MU_LSR_REG&0x01));
    r=(char)(*AUX_MU_IO_REG);
    return r=='\r'?'\n':r;
}

void uart_display_string(char* s){
    while(*s) {
        if(*s=='\n'){
            uart_send_char('\r');
        }        
        uart_send_char(*s++);
    }
}

void uart_binary_to_hex(unsigned int d) {
    unsigned int n;
    int c;
    uart_display_string("0x");
    for(c=28;c>=0;c-=4) {
        // get highest tetrad(4 bits)
        n=(d>>c)&0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n>9 ? 0x37 : 0x30;
        uart_send_char(n);
    }
}

void uart_binary_to_int(unsigned int d) {
    unsigned int n;
    int c = 0;
    while(1){
        n = d - 10*c;
        if(n<10){
            uart_send_char(n);
            break;
        }
        c = 0;
        while(n>10){
            n = n/10;
            c++;
        }
    uart_send_char(n);
    }
}

void uart_enable_interrupt() {
    uintptr_t enable_irqs1 = (uintptr_t) ENABLE_IRQS_1;
    enable_irqs1 |= AUXINIT_BIT_POSTION; // Set bit29
    mmio_write(ENABLE_IRQS_1, enable_irqs1);
}

void uart_disable_interrupt() {
    uintptr_t disable_irqs1 = (uintptr_t) DISABLE_IRQS_1;
    disable_irqs1 |= AUXINIT_BIT_POSTION; // Set bit29
    mmio_write(DISABLE_IRQS_1, disable_irqs1);
}

void set_transmit_interrupt() { *AUX_MU_IER_REG |= 0x2; }

void clear_transmit_interrupt() { *AUX_MU_IER_REG &= ~(0x2); }


void uart_handler()
{
    int RX = (*AUX_MU_IIR_REG & 0x4);   // 檢查 RX 中斷是否觸發
    int TX = (*AUX_MU_IIR_REG & 0x2);   // 檢查 TX 中斷是否觸發
    int tasknum = task_head->task_num;
    

    if (RX){
        uart_display_string("*");
        char c;
        c = uart_get_char();
        read_buf[read_buf_end++] = c;
        uart_disable_interrupt();
        *AUX_MU_IER_REG &= ~(0x1);
    }
    else if (TX)   // 如果 TX 中斷觸發
    {
        while(write_buf_start != write_buf_end){
            printf("%d", task_head->task_num);
            if(task_head->task_num != tasknum){
                break;
            }
            char c = write_buf[write_buf_start++];  
            uart_send_char(c);
            delay(10000);
        }
        uart_disable_interrupt();
        clear_transmit_interrupt();
    }
}

char uart_async_get() {

    *AUX_MU_IER_REG |= (0x1);

    while (read_buf_start == read_buf_end)
    {
        uart_enable_interrupt();
        // enable_interrupt();
    }
    char c = read_buf[read_buf_start++];
    return c;
}

void uart_async_send_string(char *str) {
    write_buf[write_buf_end++] = '\r';

    for (int i = 0; str[i]; i++){
        // printf("\rwrite_buf_end = %d\r\n", write_buf_end);
        if (write_buf_end == BUFFER_MAX_SIZE)
            write_buf_end = 0;
        write_buf[write_buf_end++] = str[i];   
    }

    if(write_buf[write_buf_end-1] != '\n'){
        write_buf[write_buf_end++] = '\n';
    }
    // uart_display_string("\r*****************************\r\n");
    while(write_buf_start != write_buf_end){
        uart_enable_interrupt();
        set_transmit_interrupt();
        // enable_interrupt();
    }
}

void test_uart_async()
{
    char buffer[BUFFER_MAX_SIZE];
    size_t index = 0;
    uart_display_string("\r");
    while(1){
        buffer[index] = uart_async_get();
        if(buffer[index]=='\n'){
            break;
        }
        index++;
    }
    uart_display_string("\r\n");
    uart_async_send_string(read_buf);

}

// This function is required by printf function
void putc ( void* p, char c)
{
	uart_send_char(c);
}