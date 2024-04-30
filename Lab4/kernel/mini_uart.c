#include "mini_uart.h"

char async_read_buf[1024];
char async_write_buf[1024];
uint32_t read_head = 0, read_end = 0;
uint32_t write_head = 0, write_end = 0;

void init_buffer(){
    for (int i = 0; i < 1024; i++){
        async_read_buf[i] = 0;
        async_write_buf[i] = 0;
    }
}

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

    if (read_end == 1024)
        read_end = 0;

    enable_interrupt();
    
}

void tx_interrupt_handler(){

    // disable_interrupt();

    while (write_head != write_end){
        while (!(*AUX_MU_LSR & 0x20))
            asm volatile("nop");

        disable_interrupt();
        char ch = async_write_buf[write_head++];
        *AUX_MU_IO = ch;

        if (write_head == 1024)
            write_head = 0;

        enable_interrupt();
    }

    // enable_interrupt();

}

void async_uart_handler(){
    disable_uart_interrupt();
    
    if (*AUX_MU_IIR & 0x04){
        // print_str("\nread interrupt");
        disable_uart_read_interrupt();
        rx_interrupt_handler();
    }else if (*AUX_MU_IIR & 0x02){
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

        if (read_head == 1024)
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

    // print_str("\nhere");

    disable_interrupt();
    
    while (*str != '\0'){
        if (*str == '\n'){
            async_write_buf[write_end++] = '\r';
            
            if (write_end == 1024)
                write_end = 0;

        }

        async_write_buf[write_end++] = *str++;

        if (write_end == 1024)
            write_end = 0;
    }

    // async_write_buf[write_end++] = '\0';

    // print_newline();
    // print_hex(write_end);
    // print_str(async_write_buf);

    enable_interrupt();

    enable_uart_write_interrupt();
}

void async_uart_hex(uint32_t dec_val){

    // print hex value
    for (int shft = 28; shft >= 0; shft -= 4){
        unsigned int n = (dec_val >> shft) & 0xf;

        n += (n >= 10 ? 0x57 : 0x30); 

        char ch = n;
        async_write_buf[write_end++] = ch;

        if (write_end == 1024)
            write_end = 0;
    }
}

void async_uart_newline(){
    async_uart_puts("\n");
}

void test_async_uart(){
    
    enable_uart_interrupt();

    char buffer[1024];

    uint32_t rlen = async_uart_gets(buffer, 1024);

    async_uart_puts(buffer);

    disable_uart_interrupt();
    
}