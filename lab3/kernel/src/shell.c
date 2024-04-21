#include "shell.h"
#include "mini_uart.h"
#include "power.h"
#include "mbox.h"
#include "cpio.h"
#include "utils.h"
#include "heap.h"
#include "dtb.h"
#include "exception.h"
#include <stddef.h>

#define CLI_MAX_CMD 8
#define MAX_ARGS 10
#define USER_STACK_SIZE 0x10000

extern char* dtb_ptr;
void* CPIO_DEFAULT_PLACE;

int cli_strcmp(const char* p1, const char* p2) {
    const unsigned char *s1 = (const unsigned char*) p1;
    const unsigned char *s2 = (const unsigned char*) p2;
    unsigned char c1, c2;

    do {
        c1 = (unsigned char) *s1++;
        c2 = (unsigned char) *s2++;
        if ( c1 == '\0' ) return c1 - c2;
    } while ( c1 == c2 );
    
    return c1 - c2;
}

void cli_read_cmd(char* buf) {
    char c = '\0';
    int idx = 0;
    
    while(1) {
        if (idx >= MAX_CMD_LEN) {
            uart_puts("Exceed the command length limit.");
            break;
        }
        c = uart_recv();
        if (c == '\n') {
            uart_puts("\r\n");
            break;
        }
        if ((int)c == 127) {  
            if (idx > 0) {
                uart_puts("\b \b");
                buf[--idx] = '\0';
            }
            continue;
        }
        if ( c > 16 && c < 32 ) continue;
        if ( c > 127 ) continue;
        buf[idx++] = c;
        uart_send(c);
    }
}

void cli_exec_cmd(char* buf) {
    char* argvs[MAX_ARGS];
    char* cmd = buf;
    int* count = 0;
    str_split(cmd, ' ', argvs, count);

    if (cli_strcmp(cmd, "cat") == 0) {
        cmd_cat(argvs[1]);
    } else if (cli_strcmp(cmd, "dtb") == 0) {
        cmd_dtb();
    } else if (cli_strcmp(cmd, "currel") == 0) {
        cmd_currentEL();
    } else if (cli_strcmp(cmd, "exec") == 0) {
        cmd_exec_program(argvs[1]);
    } else if (cli_strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (cli_strcmp(cmd, "hello") == 0) {
        cmd_hello();
    } else if (cli_strcmp(cmd, "hwinfo") == 0) {
        cmd_hwinfo();
    } else if (cli_strcmp(cmd, "reboot") == 0) {
        cmd_reboot();
    } else if (cli_strcmp(cmd, "ls") == 0) {
        cmd_ls();
    } else if (cli_strcmp(cmd, "malloc") == 0) {
        cmd_malloc();
    } else if(*cmd) {
        uart_puts(cmd);
        uart_puts(": Command not found QQ, type help to get more information.\r\n");
    }
}

void cli_clear_cmd(char* buf, int length) {
    for (int i=0; i<length; i++) {
        buf[i] = '\0';
    }
}

void cli_print_welcome_msg() {
    uart_puts("\r\n");
    uart_puts("===========================================\r\n");
    uart_puts("| * * * * * * * * * * * * * * * * * * * * |\r\n");
    uart_puts("| * * (◍•ᴗ•◍)  OSC 2024 SHELL (◍•ᴗ•◍) * * |\r\n");
    uart_puts("| * * * * * * * * * * * * * * * * * * * * |\r\n");
    uart_puts("===========================================\r\n");                    
    uart_puts("\r\n");
}

void cmd_help() {
    uart_puts("Example usage:\r\n");
    uart_puts("   help              - list all commands.\r\n");
    uart_puts("   hello             - print hello message.\r\n");
    uart_puts("   hwinfo            - print hardware info.\r\n");
    uart_puts("   currel            - print current EL.\r\n");
    uart_puts("   cat  [filePath]   - get content from a file.\r\n");
    uart_puts("   exec [filePath]   - execute a img program.\r\n");
    uart_puts("   ls                - list all files in directory.\r\n");
    uart_puts("   dtb               - show device tree.\r\n");
    uart_puts("   malloc            - test malloc function.\r\n");
    uart_puts("   reboot            - reboot the device.\r\n");
}

void cmd_hello() {
    uart_puts("Hello World!\r\n");
}

void cmd_hwinfo() {
    // print hw revision
    pt[0] = 8 * 4;
    pt[1] = MBOX_REQUEST_PROCESS;
    pt[2] = MBOX_TAG_GET_BOARD_REVISION;
    pt[3] = 4;
    pt[4] = MBOX_TAG_REQUEST_CODE;
    pt[5] = 0;
    pt[6] = 0;
    pt[7] = MBOX_TAG_LAST_BYTE;

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)) ) {
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

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)) ) {
        uart_puts("ARM Memory Base Address\t: ");
        uart_2hex(pt[5]);
        uart_puts("\r\n");
        uart_puts("ARM Memory Size\t\t: ");
        uart_2hex(pt[6]);
        uart_puts("\r\n");
    }
}

void cmd_reboot() {
    uart_puts("Reboot in 5 seconds ...\r\n\r\n");
    volatile unsigned int* rst_addr = (unsigned int*)PM_RSTC;
    *rst_addr = PM_PASSWORD | 0x20;
    volatile unsigned int* wdg_addr = (unsigned int*)PM_WDOG;
    *wdg_addr = PM_PASSWORD | 5;
}

void cmd_ls() {
    char* c_filepath;
    char* c_filedata;
    unsigned int c_filesize;
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_PLACE;

    while(header_ptr != 0) {
        int err = cpio_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        if (err) {
            uart_puts("CPIO parse error\r\n");
        }
        if (header_ptr != 0) {
            uart_puts(c_filepath);
            uart_puts("\r\n"); 
        }
    }
}

void cmd_cat(char* filepath) {
    char* c_filepath;
    char* c_filedata;
    unsigned int c_filesize;
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_PLACE;

    while(header_ptr != 0) {
        int err = cpio_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        if (err) {
            uart_puts("CPIO parse error\r\n");
            break;
        }
        if (header_ptr != 0 && strcmp(filepath, c_filepath) == 0) {
            uart_puts("%s\r\n", c_filedata);
            break; 
        }
        if (header_ptr == 0) 
            uart_puts("cat: %s: No such file or directory\n", filepath);
    }
}

void cmd_malloc() {
    //Test malloc
    char* test1 = malloc(0x22);
    strcpy(test1, "test malloc1");
    uart_puts("%s\r\n", test1);

    char* test2 = malloc(0x10);
    strcpy(test2, "test malloc2");
    uart_puts("%s\r\n", test2);

    char* test3 = malloc(0x17);
    strcpy(test3, "test malloc3");
    uart_puts("%s\r\n", test3);
}

void cmd_dtb() {
    parse_dtb_tree(dtb_ptr, dtb_callback_show_tree);
}

void cmd_exec_program(char* filepath){
    char* c_filepath;
    char* c_filedata;
    unsigned int c_filesize;
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_PLACE;

    while(header_ptr != 0) {
        int err = cpio_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        if (err) {
            uart_puts("CPIO parse error\r\n");
            break;
        }
        if (header_ptr != 0 && strcmp(filepath, c_filepath) == 0) {
            char* user_stack = malloc(USER_STACK_SIZE);
            /*
            set spsr_el1 to 0x3c0 and elr_el1 to the program’s start address.
            set the user program’s stack pointer to a proper position by setting sp_el0.
            issue eret to return to the user code. 
            */
            asm(
                "mov    x10,        0x3c0\n\t"
                "msr    spsr_el1,   x10\n\t"
                "msr    elr_el1,    %0\n\t"
                "msr    sp_el0,     %1\n\t"
                "eret   \n\t"
                :: "r"(c_filedata), "r"(user_stack + USER_STACK_SIZE)
            );
            break;
        }
        if (header_ptr == 0) 
            uart_puts("cat: %s: No such file or directory\n", filepath);
    }
}

void cmd_currentEL() {
    print_currentEL();
}