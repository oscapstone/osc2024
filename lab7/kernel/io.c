#include <bsp/uart.h>
#include <kernel/io.h>
#include <lib/stdlib.h>
#include <lib/utils.h>

char read_c() { return uart_getc(); }
void read_s(char *s) {
    char input_char;
    s[0] = 0;
    int input_index = 0;
    while ((input_char = read_c()) != '\n') {
        if (input_char == '\b' || input_char == 127) {  // 處理退格鍵
            if (input_index > 0) {      // 確保有字元可以刪除
                input_index -= 1;       // 刪除最後一個字元
                puts("\b \b");  // 在控制台上刪除這個字元，\b
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

/**
 * printf implementation using sprintf
 */
void printf(const char* fmt, ...)
{
    char buffer[1024];
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    __builtin_va_end(args);
    puts(buffer); // Assuming puts writes the string to the standard output and returns the number of characters written
}

void print_char(const char c) { return uart_send(c); }
void puts(const char *s) { return uart_puts(s); }
void print_h(const unsigned long long x) { uart_hex(x); }

void print_d(const int x) {
    char buffer[10];

    itoa(x, buffer);
    puts(buffer);
}
