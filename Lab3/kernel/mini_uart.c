#include "mini_uart.h"

char async_read_buf[256];
char async_write_buf[256];
uint32_t read_head = 0, read_end = 0;
uint32_t write_head = 0, write_end = 0;

void enable_uart_read_interrupt(){
    *AUX_MU_IER |= 0x01;
}

void disable_uart_read_interrupt(){
    *AUX_MU_IER &= (~0x01);
}

void enable_uart_write_interrupt(){
    *AUX_MU_IER |= 0x02;
}

void disable_uart_write_interrupt(){
    *AUX_MU_IER &= (~0x02);
}

void enable_uart_interrupt(){
    *ENABLE_IRQS_1 |= (1 << 29);
}

void disable_uart_interrupt(){
    *DISABLE_IRQS_1 |= (1 << 29);
}

void rx_interrupt_handler(){

    disable_interrupt();
    char ch = *((char*)AUX_MU_IO);
    async_read_buf[read_end++] = ch;

    if (read_end == 256)
        read_end = 0;

    enable_interrupt();
    
}

void tx_interrupt_handler(){

    disable_interrupt();

    while (*AUX_MU_LSR & 0x20){
        if (write_head == write_end){
            break;
        }

        char ch = async_write_buf[write_head++];
        *AUX_MU_IO = ch;

        if (write_head == 256)
            write_head = 0;

    }

    enable_interrupt();

}

void async_uart_handler(){
    disable_uart_interrupt();
    
    if (*AUX_MU_IIR & 0x04){
        // print_str("\nread interrupt");
        disable_uart_read_interrupt();
        rx_interrupt_handler();
    }else if (*AUX_MU_IIR & 0x2){
        disable_uart_write_interrupt();
        tx_interrupt_handler();
    }

    enable_uart_interrupt();
}

uint32_t async_uart_gets(char* buffer, uint32_t len){

    // disable_uart_read_interrupt();

    uint32_t read_len = 0;

    for ( ; read_len < len; ){

        while (read_head == read_end)
            enable_uart_read_interrupt();
        
        disable_interrupt();
        buffer[read_len++] = async_read_buf[read_head++];
        enable_interrupt();

        if (read_head == 256)
            read_head = 0;

        if (buffer[read_len-1] == '\n' || buffer[read_len-1] == '\r'){
            buffer[read_len] = '\0';
            // print_hex(read_end);
            break;
        }

    }

    // async_uart_puts(buffer);

    return read_len;
}

void async_uart_puts(char* str){

    disable_interrupt();
    while (*str != '\0'){
        if (*str == '\n'){
            async_write_buf[write_end++] = '\r';
            
            if (write_end == 256)
                write_end = 0;
        }

        async_write_buf[write_end++] = *str++;

        if (write_end == 256)
            write_end = 0;
    }
    enable_interrupt();

    enable_uart_write_interrupt();
}

void test_async_uart(){
    
    enable_uart_interrupt();

    char buffer[256];

    uint32_t rlen = async_uart_gets(buffer, 256);

    async_uart_puts(buffer);

    disable_uart_interrupt();
    
}