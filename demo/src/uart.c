
#include "../include/uart.h"

/**
 * Set baud rate and characteristics (115200 8N1) and map to GPIO
 */
void uart_init()
{
    register unsigned int reg;

    /* initialize UART */
    *AUX_ENABLE     |= 1;       /* enable mini UART */
    *AUX_MU_CNTL     = 0;       /* Disable transmitter and receiver during configuration. */

    *AUX_MU_IER      = 0;       /* Disable interrupt */
    *AUX_MU_LCR      = 3;       /* Set the data size to 8 bit. */
    *AUX_MU_MCR      = 0;       /* Don’t need auto flow control. */
    *AUX_MU_BAUD     = 270;     /* 115200 baud */
    *AUX_MU_IIR      = 6;       /* No FIFO */
    // *AUX_MU_IIR      = 0xc6;       /* No FIFO */

    /* map UART1 to GPIO pins */
    // GPFSEL1 register controls the function of GPIO pins
    reg = *GPFSEL1;
    reg &= ~((7<<12)|(7<<15));  /* address of gpio 14, 15 */
    reg |=   (2<<12)|(2<<15);   /* set to alt5 */

    *GPFSEL1 = reg;            
    reg=150; 
    while ( reg-- )
    {
        asm volatile("nop");
    }
    // ensures that GPIO pins are not affected by internal resistors
    *GPPUD = 0;                 /* Disable pull-up/down */
    reg=150;
    while ( reg-- )
    { 
        asm volatile("nop"); 
    }
    
    *GPPUDCLK0 = (1<<14)|(1<<15);
    reg=150; 
    while ( reg-- )
    {
        asm volatile("nop");
    }
    *GPPUDCLK0 = 0;             /* flush GPIO setup */
    //Stabilizing the Signal to ensure Proper Configuration
    reg=150; 
    while ( reg-- )
    {
        asm volatile("nop");
    }
    
    // ensures that GPIO pins are returned to a neutral state
    *GPPUDCLK0 = 0;             /* flush GPIO setup */

    *AUX_MU_CNTL = 3;           // Enable the transmitter and receiver.

    //*((volatile unsigned int*)AUX_MU_IER) &= ~0x03;

}

/**
 * Send a character
 */
void uart_send(unsigned int c)
{
    /* Wait until UART transmitter is ready to accept new data. */
    do {

        asm volatile("nop");

    } while( ! ( *AUX_MU_LSR&0x20 ));

    /* write the character to the buffer */
    *AUX_MU_IO = c;

    // ensure proper line termination
    if ( c == '\n' )
    {
        do {

            asm volatile("nop");

        } while( ! ( *AUX_MU_LSR&0x20 ));

        *AUX_MU_IO = '\r';
    }
}


/**
 * Receive a character
 */
char uart_getc() {

    char r;

    /* wait until something is in the buffer */
    do{

        asm volatile("nop");

    } while ( ! ( *AUX_MU_LSR&0x01 ) );

    /* read the data*/
    r = ( char )( *AUX_MU_IO );

    /* convert carrige return to newline */
    //standardize newline characters across different systems.
    return r == '\r' ? '\n' : r;
}

/**
 * Display a string
 */
void uart_puts(const char *s)
{
    while( *s )
    {

        uart_send(*s++);

    }
}

char *get_string(void){
    int buffer_counter = 0;
    char input_char;
    static char buffer[TMP_BUFFER_LEN];
    strset(buffer, 0, TMP_BUFFER_LEN);
    while (1) {
        input_char = uart_getc();
        if (!(input_char < 128 && input_char >= 0)) {
            uart_puts("Invalid character received\n");
            return NULL;
        } else if (input_char == LINE_FEED || input_char == CRRIAGE_RETURN) {
            uart_send(input_char);
            buffer[buffer_counter] = '\0';
            return buffer;
        }else {
            uart_send(input_char);

            if (buffer_counter < TMP_BUFFER_LEN) {
                buffer[buffer_counter] = input_char;
                buffer_counter++;
            }
            else {
                uart_puts("\nError: Input exceeded buffer length. Buffer reset.\n");
                buffer_counter = 0;
                strset(buffer, 0, TMP_BUFFER_LEN);

                // New line head
                uart_puts("# ");
            }
        }
    }
}

/**
 * Display a binary value in hexadecimal
 */
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

char read_buffer[BUFFER_SIZE];
char write_buffer[BUFFER_SIZE];

int read_index_cur = 0;
int read_index_tail = 0;
int write_index_cur = 0;
int write_index_tail = 0;

void initialize_async_buffers(void) {
    strset(read_buffer, 0, BUFFER_SIZE);
    strset(write_buffer, 0, BUFFER_SIZE);
}

void uart_send_async(char c) {
    //uart_puts("uart_send_async\n");

    write_buffer[write_index_tail++] = c;

    if (write_index_tail >= BUFFER_SIZE) {
        write_index_tail = 0;
    }

    // Enable mini UART transmit interrupt to start sending the data
    *((volatile unsigned int*)AUX_MU_IER) |= 0x02;
}

char uart_getc_async(void) {
    //*((volatile unsigned int*)AUX_MU_IER) |= (0x1);
    //uart_puts("uart_getc_async\n");
    if(read_index_cur != read_index_tail){
        //uart_puts("test");
        char c = read_buffer[read_index_cur++];
        //uart_send(c);
        if (read_index_cur >= BUFFER_SIZE) {
            read_index_cur = 0;
        }

        return c;

    }
    else{
        return '!';
    }
}

void uart_puts_async(const char *str) {
    //uart_puts("uart_puts_async\n");
    while (*str) {

        if(*str == '\n'){
            write_buffer[write_index_tail++] = '\r';
        }
        

        if (write_index_tail >= BUFFER_SIZE) {
                write_index_tail = 0;
        }

        write_buffer[write_index_tail++] = *str++;


    }
    // Enable mini UART transmit interrupt to start sending the data
    *((volatile unsigned int*)AUX_MU_IER) |= 0x02;
}

void get_string_async(char *buffer_async){
    //uart_puts("uart_string_async\n");
    int buffer_counter = 0;
    char input_char;

    while(1){
        //uart_send(buffer_counter+'0');
        input_char = uart_getc_async();
        //uart_puts("in get\n");
        // Disable mini UART transmit interrupt
        *((volatile unsigned int*)AUX_MU_IER) &= ~(0x2);

        if(input_char == '!'){
            //uart_puts("empty\n");
            continue;
        }
        //uart_puts("check input : ");
        //uart_send(input_char);
        //uart_puts("\n");
        // Get non ASCII code
        if (input_char >= 128 || input_char < 0) {
            //uart_puts("Invalid character received\n");
            continue;
        }
        if (input_char == CRRIAGE_RETURN) {
            //uart_send_async(input_char);
            buffer_async[buffer_counter] = '\0';
            //uart_puts_async("\n");
            return;
        } 
        if (buffer_counter < TMP_BUFFER_LEN - 1) {
            buffer_async[buffer_counter] = input_char;
            buffer_counter++;
            *((volatile unsigned int*)AUX_MU_IER) |= (0x1);
            //uart_puts(buffer_async);
            //uart_puts("\n");
        } else {
            //uart_puts_async("\nError: Input exceeded buffer length. Buffer reset.\n");
            buffer_counter = 0;
            strset(buffer_async, 0, TMP_BUFFER_LEN);

            // New line head
            uart_puts_async("# ");
        }
        
    }
}

void uart_int(int num){
    char tmp[65];
    itoa(num,tmp,10);
    uart_puts(tmp);
}

void uart_hex_64(uint64_t d) {
    unsigned int n;
    int c;
    for(c=60;c>=0;c-=4) {
        // get highest tetrad
        n=(d>>c)&0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n+=n>9?0x37:0x30;
        uart_send(n);
    }
}