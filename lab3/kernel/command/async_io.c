#include "../bsp/uart.h"
#include "all.h"
#include "lib/string.h"

void _async_io_demo(int argc, char **argv) {
    uart_enable_rx_interrupt();
    char command[256];

    while (1) {
        uart_async_write("\r\n(async) > ");

        while (command[strlen(command) - 1] !=
               '\n') {  // wait for enter and keep reading from buffer
            for (int i = 0; i < 2000000; i++) {
                // delay
            }
            int current_len = strlen(command);
            uart_async_read(command + current_len);
            uart_async_write(command + current_len);
            uart_enable_rx_interrupt();
        }
        command[strlen(command) - 1] = '\0';  // delete last \n
        if (strcmp(command, "exit") == 0) {
            break;
        }
        command[0] = '\0';
    }
    uart_disable_tx_interrupt();
    
    // TODO: find out why I should clear the buffer...
    uart_async_read(command);
}

struct Command async_io_demo_command = {
    .name = "async_io",
    .description = "demo async IO functions",
    .function = &_async_io_demo};
