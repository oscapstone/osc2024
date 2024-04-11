#include "../peripherals/mini_uart.h"
#include "shell.h"
#include "../peripherals/utils.h"
#include "../peripherals/mailbox.h"
#include "reboot.h"
#include "initramdisk.h"
#include "user.h"
#include "timer.h"

// List of commands.
char cmd_list[COMMAND_COUNT][SHELL_BUF_MAXSIZE] = {
    "help",
    "hello",
    "reboot",
    "board-revision",
    "get-memory-info",
    "ls",
    "cd",
    "cat",
    "loaduser",
    "setTimeout",
    "bootTime"
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
    // Iterate through all existing commands within the entire command list.
    int cmd_list_ind = 0;
    // Iterate through the cmd buffer and parse commands, flags, arguments etc.
    int cmd_ind = 0;

    // Store the arguments for commands.
    char arg[MAX_ARG_COUNT][MAX_ARG_SIZE];

    // Store the main command.
    char command[10];

    // Parse the command.
    for (; cmd_ind < SHELL_BUF_MAXSIZE; cmd_ind++) {
        if (cmd[cmd_ind] == ' ' || cmd[cmd_ind] == '\0') {
            command[cmd_ind++] = '\0';
            break;
        }

        command[cmd_ind] = cmd[cmd_ind];
    }

    // Record the number of arguments.
    int arg_cnt = 0;
    int arg_ind = 0;

    // Ends until strlen(cmd) + 1 is for capturing '\0'.
    for (; cmd_ind < strlen(cmd) + 1; cmd_ind++) {
        // End of an argument.
        if (cmd[cmd_ind] == ' ' || cmd[cmd_ind] == '\0') {
            arg[arg_cnt][arg_ind] = '\0';
            arg_cnt++;
            arg_ind = 0;

            if (cmd[cmd_ind] == '\0')
                break;
        } else {
            arg[arg_cnt][arg_ind] = cmd[cmd_ind];
            arg_ind++;
        }
    }

    // Check corresponding command within the command list.
    for (; cmd_list_ind < COMMAND_COUNT; cmd_list_ind++) {
        if (strcmp(cmd_list[cmd_list_ind], command) == 0)
            break;
    }

    // If no command was found inside command list.
    if (cmd_list_ind >= COMMAND_COUNT) {
        uart_send_string("Undefined command!\r\n");
        return;
    }

    switch(cmd_list_ind) {
        // "help" command.
        case 0:
            uart_send_string("\r\n");
            uart_send_string("  help              : print this help menu\r\n");
            uart_send_string("  hello             : print Hello World!\r\n");
            uart_send_string("  reboot            : reboot the device\r\n");
            uart_send_string("  board-revision    : print board revision\r\n");
            uart_send_string("  get-memory-info   : print ARM memory base address and size\r\n");
            uart_send_string("  ls                : print files in current directory\r\n");
            uart_send_string("  cd                : cd [dir], change the shell working directory\r\n");
            uart_send_string("  loaduser          : loaduser [file], load a user program from initial ramdisk to execute.\r\n");
            uart_send_string("\r\n");
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
            uart_send_string("\r\n");
            break;
        // Print ARM memory base address and size.
        case 4:
            get_arm_memory();
            uart_send_string("\r\n");
            break;
        // "ls" command.
        case 5:
            show_current_dir_content();
            break;
        // "cd" command.
        case 6:
            if (arg_cnt > 1) {
                uart_send_string("  cd: too many arguments\r\n");
                break;
            }
            change_directory(arg[0]);
            break;
        // "cat" command.
        case 7:
            for (int i = 0; i < arg_cnt; i++) {
                print_content(arg[i]);
            }
            break;
        // Load a user program within initramfs.
        case 8:
            if (arg_cnt > 1) {
                uart_send_string("  loaduser: too many arguments\r\n");
                break;
            }
            int i = 0;
            for (; i < userDir->nFiles; i++) {
                // Find the specified file in the current directory.
                if (strcmp(userDir->files[i]->fName, arg[0]) == 0) {
                    // Send the address of the file into the function.
                    load_user_prog((uintptr_t)userDir->files[i]);
                    break;
                }
            }

            if (i == userDir->nFiles) {
                uart_send_string("  loaduser: file not found.\r\n");
            }
            
            break;
        // Set timeout.
        case 9:
            if (arg_cnt > 2) {
                uart_send_string("  setTimeout: too many arguments\r\n");
                break;
            } else if (arg_cnt < 2) {
                uart_send_string("  setTimeout: not enough arguments\r\n");
                break;
            }

            int time = str2Int(arg[1], strlen(arg[1]));
            add_timer(print_timeout_message, (void *)arg[0], time);

            break;
        // Check the time(in secs) after booting and set the next timeout to 2 seconds.
        case 10:
            if (arg_cnt > 0) {
                uart_send_string("  bootTime: too many arguments\r\n");
                break;
            }
            
            int cur_time = get_current_time();
            uart_send_string("Time after booting: ");
            uart_send_uint(cur_time);
            uart_send_string("\r\n");
            char message[30];
            strcpy(message, "Timeout after booting\r\n");
            add_timer(print_timeout_message, message, 2);
            break;
        default:
            break;
    }

}

void shell() {

    // Buffer to store commands.
    char cmd[SHELL_BUF_MAXSIZE];

    // Acts as a pointer to the cmd buffer.
    int pos = 0;

    // To avoid printing command prompt when pressing enter with empty input.
    int press_backspace = 0;
    int press_arrow_key = 0;
    int press_space = 0;

    uart_send_string("Welcome to Josh's mini OS!\r\n");

    // Wait for user input.
    while (1) {
        // Clear buffer for new input.
        reset_buf(cmd);
        pos = 0;

        // Ready to fetch input command from the user.
        if (!press_backspace && !press_arrow_key && !press_space)
            uart_send_string("josh@raspberrypi:~ $ ");
        cmd_status = VALID_COMMAND;
        char c = uart_recv();
        
        // If a backspace character is entered. On M3 mac, 0x7F(Delete) is sent when backspace is pressed.
        // On some other configurations, 0x08(backspace) might be sent.
        if (c == 0x7F || c == 0x08) {
            press_backspace = 1;
            continue;
        // User pressed enter with no input.(Enter: 0xA, Carriage return: 0xD)
        } else if (c == 0xA || c == 0xD) {
            uart_send_string("\r\n");
            continue;
        // User pressed left arrow with no input. Arrow keys pass multi-bytes. "0xE0" and "0x4B".
        } else if (c == 0xE0 || c == 0x8B) {
            // uart_send_string("In\r\n");
            press_arrow_key = 1;
            continue;
        }

        // if (press_arrow_key) {
        //     if (c == 0x4B) {
        //         uart_send_string("In2\r\n");
        //         continue;
        //     }
        // }

        press_backspace = 0;
        press_arrow_key = 0;
        press_space = 0;
        // Print the user input on the screen.
        uart_send(c);

        // Discard all leading white spaces.
        if (c == ' ') {
            press_space = 1;
            continue;
        }

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