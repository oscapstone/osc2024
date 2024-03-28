#include "uart.h"
#include "lib_func.h"
#include "shell_func.h"
#include "initrd.h"
#include "definitions.h"


void main()
{
    // set up serial console
    uart_init();

    uart_puts("\rSimple Shell 1.0\n");
    char inputBuffer[64];
    int i = 0;
    // char currentDirectory[64];
    char* string = simple_alloc(8);

    while (1)
    {
        // Read input until newline of buffer is full
        for (i = 0; i < sizeof(inputBuffer) - 1; i++)
        {
            char c = uart_getc();
            if ((c >= 32 && c <= 126) || c == '\n') // Check for valid ASCII characters or newline
            {
                inputBuffer[i] = c;
                uart_send(inputBuffer[i]); // Echo the valid character
                if (inputBuffer[i] == '\n')
                {
                    break; // Exit the loop if newline is encountered
                }
            }
            else
            {
                i--; // Decrement the index to overwrite any invalid characters
            }
        }
        inputBuffer[i] = '\0';
        // Extract the first word from inputBuffer
        char *command = strtok(inputBuffer, " ");
        char *argument = strtok(NULL, " ");
        // Check commands
        if (strcmp(command, "help") == 0)
        {
            uart_puts("\r# help\nhelp\t: print this help menu\nhello\t: print Hello World!\nreboot\t: reboot the device\ninfo\t: get basic hardware info\nprintfs\t: print the contents of the initrd\nls\t: list the contents of the current directory\ncat\t: print the contents of a file\npwd\t: print the current directory\ncd\t: change the current directory\nclear\t: clear the screen\necho\t: print the input string\n");
        }
        else if (strcmp(command, "hello") == 0)
        {
            uart_puts("\rHello World!\n");
        }
        else if (strcmp(command, "info") == 0)
        {
            uart_puts_boardrev();
            uart_puts_armmemory();
        }
        else if (strcmp(command, "reboot") == 0){
            reset(5);
        } 
        else if(strcmp(command, "printfs") == 0){
            initrd_list();
        }
        // else if(strcmp(command, "ls") == 0){
        //     initrd_currentdir_list("");
        // }
        else if(strcmp(command, "cat") == 0){
            initrd_cat(argument);
        }
        // else if(strcmp(command, "cd") == 0){
        //     if(argument == NULL){
        //         uart_puts("\rNo directory specified\n");
        //     } else {
        //         if (strcmp(argument, "..") == 0) {
        //             // Go up to the parent directory
        //             char *last_slash = strrchr(currentDirectory, '/');
        //             if (last_slash != currentDirectory) { // Check if it's not the root directory
        //                 *last_slash = '\0';
        //             } else {
        //                 // If at root, ensure it remains "/"
        //                 currentDirectory[1] = '\0';
        //             }
        //         } else {
        //             // Go into the specified directory
        //             if (currentDirectory[strlen(currentDirectory) - 1] != '/') {
        //                 strcat(currentDirectory, "/");
        //             }
        //             strcat(currentDirectory, argument);
        //         }
        //         uart_puts("\rDirectory changed to ");
        //         uart_puts(currentDirectory);
        //         uart_puts("\n");
        //     }
        // }
        // else if(strcmp(command, "pwd") == 0){
        //     uart_puts("\rCurrent directory: ");
        //     uart_puts(currentDirectory);
        //     uart_puts("\n");
        // }
        else if(strcmp(command, "clear") == 0){
            uart_puts("\033[2J\033[H");
        }
        else if(strcmp(command, "echo") == 0){
            uart_puts("\r");
            uart_puts(argument);
            uart_puts("\n");
        }
        else {
            uart_puts("\runknown command\n");
        }

        // reset
        memset(inputBuffer, 0, sizeof(inputBuffer));
        memset(command, 0, sizeof(command));
        memset(argument, 0, sizeof(argument));
    }

}