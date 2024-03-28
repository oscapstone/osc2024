#include <stddef.h>
#include "shell.h"
#include "uart1.h"
#include "mbox.h"
#include "power.h"
#include "cpio.h"
#include "utils.h"
#include "dtb.h"
#include "heap.h"

#define CLI_MAX_CMD 8

extern char *dtb_ptr;
void *CPIO_DEFAULT_PLACE;

struct CLI_CMDS cmd_list[CLI_MAX_CMD] =
    {
        {.command = "cat", .help = "concatenate files and print on the standard output"},
        {.command = "dtb", .help = "show device tree"},
        {.command = "hello", .help = "print Hello World!"},
        {.command = "help", .help = "print all available commands"},
        {.command = "malloc", .help = "simple allocator in heap session"},
        {.command = "info", .help = "get device information via mailbox"},
        {.command = "ls", .help = "list directory contents"},
        {.command = "reboot", .help = "reboot the device"}};

void cli_cmd_clear(char *buffer, int length)
{
    for (int i = 0; i < length; i++)
    {
        buffer[i] = '\0';
    }
};

void cli_cmd_read(char *buffer)
{
    char c = '\0';
    int idx = 0;
    while (1)
    {
        if (idx >= CMD_MAX_LEN)
            break;

        c = uart_recv();
        if (c == '\n')
        {
            uart_puts("\r\n");
            break;
        }
        if (c == 127 && idx > 0)
        {
            uart_puts("\b \b");
            idx--;
        }
        else
        {
            buffer[idx++] = c;
            uart_send(c);
        }
    }
}

void cli_cmd_exec(char *buffer)
{
    if (!buffer)
        return;

    char *cmd = buffer;
    char *argvs;

    while (1)
    {
        if (*buffer == '\0')
        {
            argvs = buffer;
            break;
        }
        if (*buffer == ' ')
        {
            *buffer = '\0';
            argvs = buffer + 1;
            break;
        }
        buffer++;
    }

    if (strcmp(cmd, "cat") == 0)
    {
        cmd_cat(argvs);
    }
    else if (strcmp(cmd, "dtb") == 0)
    {
        cmd_dtb();
    }
    else if (strcmp(cmd, "hello") == 0)
    {
        cmd_hello();
    }
    else if (strcmp(cmd, "help") == 0)
    {
        cmd_help();
    }
    else if (strcmp(cmd, "info") == 0)
    {
        cmd_info();
    }
    else if (strcmp(cmd, "malloc") == 0)
    {
        cmd_malloc();
    }
    else if (strcmp(cmd, "ls") == 0)
    {
        cmd_ls(argvs);
    }
    else if (strcmp(cmd, "reboot") == 0)
    {
        cmd_reboot();
    }
}

void cli_print_banner()
{
    uart_puts("\r\n");
    uart_puts("=======================================\r\n");
    uart_puts("                 Shell                 \r\n");
    uart_puts("=======================================\r\n");
}

// Function to display the contents of a file in the CPIO archive
void cmd_cat(char *filepath)
{
    char *c_filepath;                                         // Pointer to the current file's path in the CPIO archive
    char *c_filedata;                                         // Pointer to the current file's data in the CPIO archive
    unsigned int c_filesize;                                  // Size of the current file in the CPIO archive
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_PLACE; // Pointer to the current file header in the CPIO archive

    while (header_ptr != 0) // Loop through all files in the CPIO archive
    {
        // Parse the header of the current file
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        // If there is an error in parsing the header
        if (error)
        {
            uart_puts("cpio parse error");
            break;
        }

        // If the current file's path matches the requested filepath
        if (strcmp(c_filepath, filepath) == 0)
        {
            uart_puts("%s", c_filedata); // Print the file data
            break;
        }

        // If this is the last file in the CPIO archive (TRAILER!!!)
        if (header_ptr == 0)
            uart_puts("cat: %s: No such file or directory\n", filepath);
    }
}

// Function to list all files in the CPIO archive
void cmd_ls(char *workdir)
{
    char *c_filepath;                                         // Pointer to the current file's path in the CPIO archive
    char *c_filedata;                                         // Pointer to the current file's data in the CPIO archive
    unsigned int c_filesize;                                  // Size of the current file in the CPIO archive
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_PLACE; // Pointer to the current file header in the CPIO archive

    while (header_ptr != 0) // Loop through all files in the CPIO archive
    {
        // Parse the header of the current file
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        // If there is an error in parsing the header
        if (error)
        {
            uart_puts("cpio parse error");
            break;
        }

        // If this is not the last file in the CPIO archive (not TRAILER!!!)
        if (header_ptr != 0)
            uart_puts("%s\n", c_filepath); // Print the file path
    }
}

// Function to traverse and print the device tree
void cmd_dtb()
{
    traverse_device_tree(dtb_ptr, dtb_callback_show_tree);
}

// Function to display a list of available commands
void cmd_help()
{
    for (int i = 0; i < CLI_MAX_CMD; i++) // Loop through all commands in the command list
    {
        uart_puts(cmd_list[i].command); // Print the command name
        uart_puts("\t\t: ");
        uart_puts(cmd_list[i].help); // Print the help text for the command
        uart_puts("\r\n");
    }
}

void cmd_hello()
{
    uart_puts("Hello World!\r\n");
}

void cmd_info()
{
    // print hw revision
    pt[0] = 8 * 4;
    pt[1] = MBOX_REQUEST_PROCESS;
    pt[2] = MBOX_TAG_GET_BOARD_REVISION;
    pt[3] = 4;
    pt[4] = MBOX_TAG_REQUEST_CODE;
    pt[5] = 0;
    pt[6] = 0;
    pt[7] = MBOX_TAG_LAST_BYTE;

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)))
    {
        uart_puts("Hardware Revision\t: ");
        uart_2hex(pt[6]);
        uart_2hex(pt[5]);
        uart_puts("\r\n");
    }
    // print arm memory
    pt[0] = 8 * 4;
    pt[1] = MBOX_REQUEST_PROCESS;
    pt[2] = MBOX_TAG_GET_ARM_MEMORY;
    pt[3] = 8;
    pt[4] = MBOX_TAG_REQUEST_CODE;
    pt[5] = 0;
    pt[6] = 0;
    pt[7] = MBOX_TAG_LAST_BYTE;

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)))
    {
        uart_puts("ARM Memory Base Address\t: ");
        uart_2hex(pt[5]);
        uart_puts("\r\n");
        uart_puts("ARM Memory Size\t\t: ");
        uart_2hex(pt[6]);
        uart_puts("\r\n");
    }
}

void cmd_malloc()
{
    // Test malloc by allocating memory and copying strings into the allocated memory
    char *test1 = malloc(0x18);                  // Allocate 24 bytes of memory
    memcpy(test1, "malloc1", sizeof("malloc1")); // Copy the string "malloc1" into the allocated memory
    uart_puts("%s\n", test1);                    // Print the string stored in the allocated memory

    char *test2 = malloc(0x20);                  // Allocate 32 bytes of memory
    memcpy(test2, "malloc2", sizeof("malloc2")); // Copy the string "malloc2" into the allocated memory
    uart_puts("%s\n", test2);                    // Print the string stored in the allocated memory

    char *test3 = malloc(0x28);                  // Allocate 40 bytes of memory
    memcpy(test3, "malloc3", sizeof("malloc3")); // Copy the string "malloc3" into the allocated memory
    uart_puts("%s\n", test3);                    // Print the string stored in the allocated memory
}

void cmd_reboot()
{
    uart_puts("Reboot in 5 seconds ...\r\n\r\n");
    volatile unsigned int *rst_addr = (unsigned int *)PM_RSTC;
    *rst_addr = PM_PASSWORD | 0x20;
    volatile unsigned int *wdg_addr = (unsigned int *)PM_WDOG;
    *wdg_addr = PM_PASSWORD | 5;
}
