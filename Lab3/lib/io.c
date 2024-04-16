#include "io.h"

void print_char(char ch){
    uart_write(ch);
}

// Display String
void print_str(char* str) {

    while(*str) {

        /* convert newline to carrige return + newline */
        if(*str == '\n')
            uart_write('\r');
        uart_write(*str++);
    }
}

void print_hex(uint32_t dec_val){

    // print hex value
    for (int shft = 28; shft >= 0; shft -= 4){
        unsigned int n = (dec_val >> shft) & 0xf;

        n += (n >= 10 ? 0x57 : 0x30); 

        char ch = n;
        uart_write(ch);
    }
}

void print_nchar(char* str, int len){
    for (int i = 0; i < len; i++){
        print_char(str[i]);
    }
}

void print_newline(){
    print_str("\n");
}

// Get the command from user prompt
void read_input(char* buffer){
    char input_ch;

    buffer[0] = 0;
    int str_ind = 0;

    while((input_ch = uart_read()) != '\n'){
        buffer[str_ind++] = input_ch;

        if (input_ch <= 126 && input_ch >= 32)
            print_char(input_ch);
    }

    buffer[str_ind] = 0;
}