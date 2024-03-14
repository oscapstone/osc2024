#include "shell.h"
#include "command.h"
#include "mini_uart.h"
#include "string.h"

void shell_start() {
    int buffer_counter = 0;
    char input_char;
    char buffer[MAX_BUFFER_LEN];
    enum SPECIAL_CHARACTER input_parse;

    // Re-initialize UART after reboot
    uart_init();
    uart_flush();

    strset(buffer, 0, MAX_BUFFER_LEN);

    // new line head
    uart_puts("# ");

    // read input
    while (1) {
        input_char = uart_getc(); // Read a character from UART

        input_parse = parse(input_char); // Parse the input character

        // Process the command based on the parsed input
        command_controller(input_parse, input_char, buffer, &buffer_counter);
    }
}

// Function to parse special characters in the input
enum SPECIAL_CHARACTER parse(char c) {
    if (!(c < 128 && c >= 0)) // checks if the character c is outside the range of ASCII characters (0 to 127)
        return UNKNOWN;

    if (c == BACK_SPACE)
        return BACK_SPACE;
    else if (c == LINE_FEED || c == CARRIAGE_RETURN) // checks if the character c is either a line feed (LF) or a carriage return (CR). 
    //These characters are used to indicate the end of a line of text
        return NEW_LINE;
    else
        return REGULAR_INPUT;
}

// Function to control the command execution based on input
void command_controller(enum SPECIAL_CHARACTER input_parse, char c, char buffer[], int* counter) {
    if (input_parse == UNKNOWN)
        return;

    // Handle backspace
    if (input_parse == BACK_SPACE) {
        if ((*counter) > 0) {
            (*counter)--;
            uart_putc('\b');   // Move the cursor back
            uart_putc(' ');    // Erase the character on the terminal
            uart_putc('\b');   // Move the cursor back again
        }
    } 
    // Handle new line
    else if (input_parse == NEW_LINE) { 
        uart_putc(c);

        if ((*counter) == MAX_BUFFER_LEN) {
            input_buffer_overflow_message(buffer);
        } else {
            buffer[(*counter)] = '\0';

            // Execute the appropriate command based on the input buffer
            if (!strcmp(buffer, "help"))
                command_help();
            else if (!strcmp(buffer, "hello"))
                command_hello();
            else if (!strcmp(buffer, "reboot"))
                command_reboot();
            else if (!strcmp(buffer, "info"))
                command_info();
            else if (!strcmp(buffer, "clear"))
                command_clear();
            else
                command_not_found(buffer);
        }

        (*counter) = 0;
        strset(buffer, 0, MAX_BUFFER_LEN); // Reset buffer

        uart_puts("# "); // Print prompt for next input
    } 
    // Handle regular input
    else if (input_parse == REGULAR_INPUT) {
        uart_putc(c); // Echo the character

        // Add character to buffer if there's space
        if (*counter < MAX_BUFFER_LEN) {
            buffer[*counter] = c;
            (*counter)++;
        }
    }
}
