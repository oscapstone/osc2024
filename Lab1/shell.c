#include "include/mini_uart.h"
#include "include/shell.h"
#include "include/utils.h"
#include "include/mailbox.h"
#include "include/reboot.h"

// List of commands.
char cmd_list[COMMAND_COUNT][SHELL_BUF_MAXSIZE] = {
    {'h', 'e', 'l', 'p', '\0'},
    {'h', 'e', 'l', 'l', 'o', '\0'},
    {'r', 'e', 'b', 'o', 'o', 't', '\0'},
    {'b', 'o', 'a', 'r', 'd','-', 'r', 'e', 'v', 'i', 's', 'i', 'o', 'n', '\0'},
    {'g', 'e', 't', '-', 'm', 'e', 'm', 'o', 'r', 'y', '-', 'i', 'n', 'f', 'o','\0'}
};

// If 0, command is valid, otherwise invalid.
int cmd_status;

void reset_buf(char* buf) {
    for (int i = 0; i < SHELL_BUF_MAXSIZE; i++) {
        buf[i] = 0;
    }
}

// Parse the entire command list to check which command the user inputs.
void handle_command(char* cmd) {
    int cmd_ind = 0;

    for (; cmd_ind < COMMAND_COUNT; cmd_ind++) {
        if (strcmp(cmd_list[cmd_ind], cmd))
            break;
    }

    // If no command was found inside command list.
    if (cmd_ind >= COMMAND_COUNT) {
        uart_send_string("Undefined command!\r\n");
        return;
    }

    switch(cmd_ind) {
        // "help" command.
        case 0:
            uart_send_string("help              : print this help menu\r\n");
            uart_send_string("hello             : print Hello World!\r\n");
            uart_send_string("reboot            : reboot the device\r\n");
            uart_send_string("board-revision    : print board revision\r\n");
            uart_send_string("get-memory-info   : print ARM memory base address and size\r\n");
            break;
        // print "hello".
        case 1:
            uart_send_string("Hello World!\r\n");
            break;
        // Reboot RPi3.
        case 2:
            reset(1000);
            break;
        // Print board revisiion.
        case 3:
            get_board_revision();
            break;
        // Print ARM memory base address and size.
        case 4:
            get_arm_memory();
            break;
        default:
            break;
    }

}

void shell(void) {

    // Buffer to store commands.
    char cmd[SHELL_BUF_MAXSIZE];

    // Acts as a pointer to the cmd buffer.
    int pos = 0;

    uart_send_string("\r\nWelcome to Josh's mini OS!\r\n");

    // Wait for user input.
    while (1) {
        // Clear buffer for new input.
        reset_buf(cmd);
        pos = 0;

        // Ready to fetch input command from the user.
        uart_send_string("josh@raspberrypi:~ $ ");
        cmd_status = VALID_COMMAND;
        char c = uart_recv();

        // If a backspace character is entered. On M3 mac, 0x7F(Delete) is sent when backspace is pressed.
        // On some other configurations, 0x08(backspace) might be sent.
        if (c == 0x7F || c == 0x08)
            continue;

        // Print the user input on the screen.
        uart_send(c);
        // Fetch all characters the user inputs.
        while ((c != '\n') && (c != '\r')) {
            if (c == 0x7F || c == 0x08) {
                if (pos) {
                    cmd[pos--] = '\0';
                    uart_send('\b');
                    uart_send(' ');
                    uart_send('\b');
                }
            } else {
                cmd[pos++] = c;
                
                // Print error message if user inputs too many characters.
                if (pos >= SHELL_BUF_MAXSIZE) {
                    cmd_status = COMMAND_EXCEED_SIZELIMIT;
                    reset_buf(cmd);
                    pos = 0;
                    uart_send_string("Exceed input command size!\r\n");
                    break;
                }
            }
            c = uart_recv();
            uart_send(c);
        }

        uart_send_string("\r\n");

        if (cmd_status != VALID_COMMAND)
            continue;

        // Add an ending string character to the input command.
        cmd[pos] = '\0';

        handle_command(cmd);
    }
}