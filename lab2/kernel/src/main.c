#include "uart1.h"
#include "shell.h"
#include "heap.h"
#include "utils.h"
#include "dtb.h"

extern char *dtb_ptr; // External variable to hold the Device Tree Blob (DTB) pointer

void main(char *arg)
{
    char input_buffer[CMD_MAX_LEN]; // Buffer to hold user input for shell commands

    dtb_ptr = arg;                                         // Store the DTB pointer passed by the bootloader in the global variable
    traverse_device_tree(dtb_ptr, dtb_callback_initramfs); // Traverse the device tree to initialize the initramfs

    uart_init();                                // Initialize UART for input/output
    uart_puts("loading dtb from: 0x%x\n", arg); // Print the address from which the DTB is loaded
    cli_print_banner();                         // Print a welcome banner for the shell

    while (1)
    {                                             // Main shell loop
        cli_cmd_clear(input_buffer, CMD_MAX_LEN); // Clear the input buffer
        uart_puts("# ");                          // Print a prompt for the user
        cli_cmd_read(input_buffer);               // Read a command from the user
        cli_cmd_exec(input_buffer);               // Execute the command
    }
}
