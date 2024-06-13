#include "kernel/uart.h"
#include "kernel/gpio.h"
#include "kernel/utils.h"

char read_buffer[MAX_BUF_LEN];
char write_buffer[MAX_BUF_LEN];
int read_index_cur = 0;
int read_index_tail = 0;
int write_index_cur = 0;
int write_index_tail = 0;

void uart_init (void){
    // allocate an 32 bits register(if we don't assign resiter, it would be allocated in memory)
    register unsigned int reg;

    *AUX_ENABLE         |=  1;      // enable mini UART. Then mini UART register can be accessed.
    *AUX_MU_CNTL_REG    =   0;      // Disable transmitter and receiver during configuration.
    *AUX_MU_IER_REG     =   0;      // Disable interrupt because currently you don’t need interrupt.
    *AUX_MU_LCR_REG     =   3;      // Set the data size to 8 bit.
    *AUX_MU_MCR_REG     =   0;      // Don’t need auto flow control.
    *AUX_MU_BAUD_REG    =   270;    // Set baud rate to 115200
    *AUX_MU_IIR_REG     =   6;      // No FIFO

    // p.92
    //reg = mmio_read(GPFSEL1);
    reg = *GPFSEL1;
    // clear GPIO14,15(~7 = 000)
    reg &= ~((7<<12) | (7<<15));    // 14-12 bits are for gpio14, 17-15 are fir gpio15
    reg |= (2<<12) | (2<<15);       // Assert: set to ALT5 for mini UART, while ALT0 is for PL011 UART
    
    // set GPIO14, 15 to miniUART
    //mmio_write(GPFSEL1, reg);
    *GPFSEL1 = reg;

    // p.101
    *GPPUD              =  0;       // Write to GPPUD to set the required control signal (i.e. Pull-up or Pull-Down or neither to remove the current Pull-up/down)

    reg = 150;
    // Wait 150 cycles – this provides the required set-up time for the control signal
    while(reg--){
        asm volatile("nop"); 
    }

    //mmio_write(GPPUDCLK0, (1 << 14) | (1 << 15));
    *GPPUDCLK0 = (1 << 14) | (1 << 15); // 1 = Assert Clock on line

    reg = 150;
    // Wait 150 cycles – this provides the required set-up time for the control signal
    while(reg--){
        asm volatile("nop"); 
    }

    // Write to GPPUDCLK0/1 to remove the clock 
    //mmio_write(GPPUDCLK0, 0);
    *GPPUDCLK0 = 0;
    reg = 150;
    // Wait 150 cycles – this provides the required set-up time for the control signal
    while(reg--){
        asm volatile("nop"); 
    }
    *AUX_MU_CNTL_REG    =   3;      // Enable the transmitter and receiver.
}

void uart_putc(unsigned char c){
    // p.15, bit 5 is set if the transmit FIFO can accept at least one byte.
    // 0x20 = 0010 0000
    while(!((*AUX_MU_LSR_REG) & 0x20) ){
        // if bit 5 is set, break and return IO_REG
        asm volatile("nop");
    }
    //  p.11
    //mmio_write(AUX_MU_IO_REG, c);
    *AUX_MU_IO_REG = c;
    // If no CR, first line of output will be moved right for n chars(n=shell command just input), not sure why
    if(c == '\n'){
        while(!((*AUX_MU_LSR_REG) & 0x20) ){
            asm volatile("nop");
        }
        *AUX_MU_IO_REG = '\r';
    }
}

unsigned char uart_getc(){
    char r;
    // p.15, bit 0 is set if the receive FIFO holds at least 1 symbol.
    while(!((*AUX_MU_LSR_REG) & 0x01) ){
        // if bit 0 is set, break and return IO_REG
        asm volatile("nop");
    }
    //  p.11
    //r =  (char)(mmio_read(AUX_MU_IO_REG));
    r = (char)(*AUX_MU_IO_REG);
    /* convert carriage return to newline */
    return r=='\r'?'\n':r;
}

unsigned int uart_puts(const char* str){
    // I thought this 'for' usage can't be in C
    //for(int i = 0; str[i] != '\0'; i++)
    int i;
    for(i = 0; str[i] != '\0'; i++){
        if(str[i] == '\n')
            uart_putc('\r');
        uart_putc((char)str[i]);
    }

    return i;
}

void uart_puts_fixed(const char *str, int len){
    int i;
    for(i = 0; i < len; i++){
        //if(str[i] == '\n')
        //    uart_putc('\r');
        uart_putc((char)str[i]);
    }
}

void uart_itoa(int num){
    char str[12];
    int i = 0;
    int j = 0;
    int is_negative = 0;

    if(num < 0){
        is_negative = 1;
        num = -num;
    }

    do{
        str[i++] = num % 10 + '0';
        num /= 10;
    }while(num);

    if(is_negative)
        str[i++] = '-';

    str[i] = '\0';

    for(j = i - 1; j >= 0; j--){
        uart_putc(str[j]);
    }
}

void uart_b2x(unsigned int b){
    int i;
    unsigned int t;
    uart_puts("0x");
    // take [32,29] then [28,25] ...
    for(i = 28; i >=0; i-=4){
        // this is the equivalent to following method, as '0' = 0x30 and 0x37 + 10 = 'A'
        // thus convert to ASCII
        t = (b >> i) & 0xF;
        t += (t > 9 ? 0x37:0x30);
        uart_putc(t);

        // preserver right 4 bits info, others turned to 0
        /*t = (b >> i) & 0xF;
        if(t > 9){
            switch(t){
                case 10:
                    uart_putc('A');
                    break;
                case 11:
                    uart_putc('B');
                    break;
                case 12:
                    uart_putc('C');
                    break;
                case 13:
                    uart_putc('D');
                    break;
                case 14:
                    uart_putc('E');
                    break;
                case 15:
                    uart_putc('F');
                    break;    
            }
        }
        else{
            uart_putc(t + '0');
        }*/
    }
}

void uart_b2x_64(unsigned long long b){
    int i;
    unsigned long long t;
    uart_puts("0x");
    // take [32,29] then [28,25] ...
    for(i = 60; i >=0; i-=4){
        // this is the equivalent to following method, as '0' = 0x30 and 0x37 + 10 = 'A'
        // thus convert to ASCII
        t = (b >> i) & 0xF;
        t += (t > 9 ? 0x37:0x30);
        uart_putc(t);
    }
}
int uart_get_fn(char *buf){
    int buf_index = 0;
    char input_char;

    while(1){
        input_char = uart_getc();
        // Get non ASCII code
        if(input_char > 127 || input_char < 0){
            //uart_puts("\nwarning: Get non ASCII code\n");
            continue;
        }
        
        if(buf_index < MAX_BUF_LEN)
            buf[buf_index++] = parse(input_char);
        // should replace with parsed char
        uart_putc(input_char);
        // when receving ENTER
        if(input_char == '\n'){
            // add EOF after '\n'
            buf[buf_index] = '\0';
            break;
        }
    }

    return buf_index;
}

int uart_gets(char *buf, char **argv){
    int buf_index = 0;
    int argv_buf_index[5] = {0};
    int flag = -1;       // for argv
    char input_char;

    while(1){
        input_char = uart_getc();
        // Get non ASCII code
        if(input_char > 127 || input_char < 0){
            //uart_puts("\nwarning: Get non ASCII code\n");
            continue;
        }
        //uart_putc(input_char);

        if(input_char == ' '){
            if(flag >= 0)
                argv[flag][argv_buf_index[flag]] = '\0';
            flag++;

            string_set(argv[flag], 0, MAX_ARGV_LEN);
        }
        if(flag == -1){
            if(buf_index < MAX_BUF_LEN)
                buf[buf_index++] = parse(input_char);
            // should replace with parsed char
            uart_putc(input_char);
            // when receving ENTER
            if(input_char == '\n'){
                // add EOF after '\n'
                buf[buf_index] = '\0';
                break;
            }
        }
        else{
            if(argv_buf_index[flag] < MAX_ARGV_LEN && input_char != ' ')
                argv[flag][argv_buf_index[flag]++] = parse(input_char);
            uart_putc(input_char);

            if(input_char == '\n'){
                // add EOF after '\n'
                argv[flag][argv_buf_index[flag]] = '\0';
                break;
            }
        }
    }

    return buf_index;
}

int uart_irq_gets(char *buf){
    int buf_index = 0;
    char input_char;

    while(1){
        input_char = (char)uart_irq_getc();
        //uart_putc(input_char);
        if(input_char == -1)
            continue;
        // Get non ASCII code
        if(input_char > 127 || input_char < 0){
            //uart_puts("\nwarning: Get non ASCII code\n");
            continue;
        }
        if(buf_index < MAX_BUF_LEN)
            buf[buf_index++] = parse(input_char);
        // should replace with parsed char
        //uart_putc(input_char);
        // when receving ENTER
        if(input_char == '\n'){
            // add EOF after '\n'
            buf[buf_index] = '\0';
            break;
        }
    }

    return buf_index;
}

void uart_irq_on(){
    *AUX_MU_IER_REG     |=   1;  //enable receive interrupt(transmit will be handled in its function)
    *AUX_MU_IER_REG     |=   2;  //enable transmit interrupt
    *Enable_IRQs_1      |=   (1<<29);
}

void uart_irq_off(){
    mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG & ~(0x1));  //disable receive interrupt
    mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG & ~(0x2));  //disable transmit interrupt
    *Disable_IRQs_1     |=   (1<<29);
}

int uart_irq_getc(){
    uart_irq_on();
    while(1){
        // there's char in buffer
        if(read_index_cur != read_index_tail){
            int_off();
            int c = (int)read_buffer[read_index_cur++];
            // make it circular
            read_index_cur = read_index_cur % MAX_BUF_LEN;
            int_on();
            return c;
        }
        else
            unlock();
    }
    // else
    //     return -1;
}

void uart_irq_putc(unsigned char c){
    int_off();

    write_buffer[write_index_tail++] = c;
    write_index_tail = write_index_tail % MAX_BUF_LEN;
    
    int_on();
    uart_irq_on();
    //p.12 The AUX_MU_IER_REG register is primary used to enable interrupts 
    mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG | 0x2);
}

void uart_irq_puts(const char *str){
    int i;
    int_off();
    for(i = 0; str[i] != '\0'; i++){
        if(str[i] == '\n'){
            write_buffer[write_index_tail++] = '\r';
            write_index_tail = write_index_tail % MAX_BUF_LEN;
        }
        write_buffer[write_index_tail++] = str[i];
        write_index_tail = write_index_tail % MAX_BUF_LEN;
    }
    int_on();

    mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG | 0x2);
}