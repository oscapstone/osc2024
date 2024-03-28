#include "io.h"

#include "bsp/uart.h"

char read_c() { return uart_getc(); }
void read_s(char *s) {
    char input_char;
    s[0] = 0;
    int input_index = 0;
    while ((input_char = read_c()) != '\n') {
        if (input_char == '\b' || input_char == 127) {  // 處理退格鍵
            if (input_index > 0) {      // 確保有字元可以刪除
                input_index -= 1;       // 刪除最後一個字元
                print_string("\b \b");  // 在控制台上刪除這個字元，\b
                                        // 是回退一格，空格刪除字元，再一個 \b
                                        // 是將光標回退一格。
                continue;
            }
        }
        s[input_index] = input_char;
        input_index += 1;
        print_char(input_char);
    }

    s[input_index] = 0;
}
void print_char(char c) { return uart_send(c); }
void print_string(char *s) { return uart_puts(s); }

void print_h(int x) { uart_hex(x); }
