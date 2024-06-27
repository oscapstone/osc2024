#include "../include/my_string.h"
#include "../include/shell.h"
#include "../include/uart.h"
#include "../include/command.h"
#include "../include/my_stdint.h"
#include "../include/my_stdlib.h"
#include "../include/timer.h"

void shell_start(void) {
    int buffer_counter = 0;
    char input_char;
    char buffer[MAX_BUFFER_LEN];

    strset(buffer, 0, MAX_BUFFER_LEN);

    char *argv[5];
    int buf_index;
    //char input_char;
    for(buf_index = 0; buf_index < 5; buf_index++){
        argv[buf_index] = simple_malloc(MAX_ARGV_LEN);
        strset(argv[buf_index], 0, MAX_ARGV_LEN);
    }
    // New line head
    uart_puts("# ");

    // Read input
    while (1) {
        input_char = uart_getc();
        command_controller(input_char, buffer, &buffer_counter, argv);
    }
}

void command_controller(char c, char buffer[], int *counter, char **argv) {

    if (!(c < 128 && c >= 0)) {
        uart_puts("Invalid character received\n");
        return;
    } else if (c == LINE_FEED || c == CRRIAGE_RETURN) {
        uart_send(c);

        buffer[*counter] = '\0';
        int argc = parse_arguments(buffer, argv);
        
        if (argc > 0) {
            execute_command(argc, argv);
        }

        *counter = 0;
        strset(buffer, 0, MAX_BUFFER_LEN);

        // New line head
        uart_puts("# ");
    } else if (c == BACKSPACE) {   ///not working 
        uart_puts("Error: Command not found\n");
        if (*counter > 0) {
            uart_send('h');
            uart_send('\b'); // Send Backspace to move cursor back
            uart_send(' ');  // Send space to clear the character on the screen
            uart_send('\b'); // Send Backspace again to move cursor back
            (*counter)--; // Decrement counter to remove the last character from the buffer
        }
    }else {
        uart_send(c);

        if (*counter < MAX_BUFFER_LEN) {
            buffer[*counter] = c;
            (*counter)++;
        }
        else {
            uart_puts("\nError: Input exceeded buffer length. Buffer reset.\n");
            *counter = 0;
            strset(buffer, 0, MAX_BUFFER_LEN);

            // New line head
            uart_puts("# ");
        }
    }
}

void execute_command(int argc, char **argv) {
    for (command_t *cmd = commands; cmd->command != NULL; cmd++) {
        if (strcmp(argv[0], cmd->command) == 0) {
            cmd->handler(argc,argv);
            return;
        }
    }

    uart_puts("Error: Command not found\n");
    uart_puts("Try command: help\n");
}

int parse_arguments(char *buffer, char **argv) {
    int argc = 0;
    char *token = strtok(buffer, " "); // Tokenize buffer by space

    while (token != NULL && argc < 5) {
        strcpy(argv[argc], token); // Copy token to argv
        token = strtok(NULL, " ");
        argc++;
    }

    return argc;
}