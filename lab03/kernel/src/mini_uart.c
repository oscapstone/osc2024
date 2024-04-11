#include "gpio.h"
#include "mini_uart.h"
#include "string.h"
#include "irq.h"

void uart_init()
{
    // set the alternate function for GPIO14 and GPIO15
    unsigned int selector;
    selector = *GPFSEL1;
    selector &= ~(7<<12); // size gpio14 to 0 for gpio14 is bits 12-14 
    selector |= 2<<12; // set to alt5 for gpio14
    selector &= ~(7<<15); // size gpio15 to 0 for gpio15 is bits 15-17
    selector |= 2<<15; // set to alt5 for gpio15
    *GPFSEL1 = selector;

    // GPIO initilaization
    *GPPUD = 0; // disable pull up/down for all GPIO pins
    unsigned int i = 150;
    while(i--) asm volatile("nop");
    *GPPUDCLK0 = (1<<14) | (1<<15); // enable clock for gpio14 and gpio15
    i = 150;
    while(i--) asm volatile("nop");
    *GPPUDCLK0 = 0; // disable clock for gpio14 and gpio15

    // initialize mini uart
    *AUX_ENABLES = 1;        //Enable mini uart (this also enables access to its registers)
    *AUX_MU_CNTL_REG = 0;   //Disable auto flow control and disable receiver and transmitter (for now)
    *AUX_MU_IER_REG = 0;    //Disable receive and transmit interrupts
    *AUX_MU_LCR_REG = 3;    //Enable 8 bit mode
    *AUX_MU_MCR_REG = 0;    //Set RTS line to be always high
    *AUX_MU_BAUD_REG = 270; // 115200 baud
    *AUX_MU_CNTL_REG = 3;   //Finally, enable transmitter and receiver
}

char uart_recv()
{
    // char r;
    // bit 0 == 1, if receiver holds valid byte
    // do{asm volatile("nop");}while(!(*AUX_MU_LSR_REG & 0x01));
    char r;
    // bit 0 == 1, if receiver holds valid byte
    do{asm volatile("nop");}while(!(*AUX_MU_LSR_REG & 0x01));
    r = (char)(*AUX_MU_IO_REG);
    return r == '\r'?'\n':r;
    
}

void uart_send(char c)
{
    // bit 6 == 1, if transmitter is empty
    while (1) {
        if ((*AUX_MU_LSR_REG)&0x20) break;
    }
    *AUX_MU_IO_REG = c;

}

void uart_send_string(const char* str)
{
    while(*str){
        if(*str == '\n')
            uart_send('\r');
        uart_send(*str++);
    }
}

// ===============================================

char read_buff[BUFF_SIZE];
char write_buff[BUFF_SIZE];
int read_front = 0, read_rear = 0;
int write_front = 0, write_rear = 0;

void enable_uart_interrupt()
{
    enable_uart_recv_interrupt();
    *ENABLE_IRQS_1 |= AUX_INT;
}

void disable_uart_interrupt()
{
    disable_uart_trans_interrupt();
    *DISABLE_IRQS_1 |= AUX_INT;
}

void enable_uart_recv_interrupt()   // interrupt will be triggered when the receiver holds at least 1 byte
{
    *AUX_MU_IER_REG |= ENABLE_RCV_IRQ;
}

void enable_uart_trans_interrupt()  // interrupt will be triggered when the transmitter can accept at least 1 byte
{
    *AUX_MU_IER_REG |= ENABLE_TRANS_IRQ;
}

void disable_uart_recv_interrupt()
{
    *AUX_MU_IER_REG &= ~ENABLE_RCV_IRQ;
}

void disable_uart_trans_interrupt()
{
    *AUX_MU_IER_REG &= ~ENABLE_TRANS_IRQ;
}


void async_uart_handler()
{
    disable_uart_interrupt();

    unsigned int status = *AUX_MU_IIR_REG;

    if(status & AUX_MU_IIR_REG_READ) // read
    {
        char c = (char)(*AUX_MU_IO_REG);
        read_buff[read_rear++] = c;
        read_rear %= BUFF_SIZE;       
    }
    else if(status & AUX_MU_IIR_REG_WRITE) // write
    {
        while(*AUX_MU_LSR_REG & 0x20) // 0010_0000 (bit 5 == 1) if FIFO can accept at least 1 byte
        {
            if(write_front == write_rear)
            {
                enable_uart_recv_interrupt();
                break;
            }
            char c = write_buff[write_front++];
            *AUX_MU_IO_REG = c;
            write_front %= BUFF_SIZE;
        }
    }
    enable_uart_interrupt();
}

int async_uart_gets(char* buff, int size)   // get the string from the buffer
{
    int i = 0;
    for(; i<size; i++)
    {
        while(read_front == read_rear)
            asm volatile("nop");
        char c = read_buff[read_front++];
        read_front %= BUFF_SIZE;
        if(c == '\r' || c == '\n')
        {
            break;
        }
        buff[i] = c;
    }
    buff[i] = '\0';
    return i;
}

void async_uart_puts(char* buff)    // puts the string into the write buffer and enable the transmitter interrupt
{
    unsigned int len = strlen(buff);
    if(len == 0 || len >= BUFF_SIZE)
        return;
    for(int i=0; buff[i]!='\0'; i++)
    {
        write_buff[write_rear++] = buff[i];
        write_rear %= BUFF_SIZE;
    }
    enable_uart_trans_interrupt();
}