#include "../include/my_string.h"
#include "../include/shell.h"
#include "../include/uart.h"
#include "../include/command.h"

void shell_start(void) {
    int buffer_counter = 0;
    char input_char;
    char buffer[MAX_BUFFER_LEN];

    strset(buffer, 0, MAX_BUFFER_LEN);

    // New line head
    uart_puts("# ");

    // Read input
    while (1) {
        input_char = uart_getc();
        command_controller(input_char, buffer, &buffer_counter);
    }
}

void command_controller(char c, char buffer[], int *counter) {
    if (!(c < 128 && c >= 0)) {
        uart_puts("Invalid character received\n");
        return;
    } else if (c == LINE_FEED || c == CRRIAGE_RETURN) {
        uart_send(c);

        buffer[*counter] = '\0';
        execute_command(buffer);

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

void execute_command(char *buffer) {
    for (command_t *cmd = commands; cmd->command != NULL; cmd++) {
        if (strcmp(buffer, cmd->command) == 0) {
            cmd->handler();
            return;
        }
    }

    uart_puts("Error: Command not found\n");
    uart_puts("Try command: help\n");
}