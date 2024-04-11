#include "shell.h"
#include "mini_uart.h"
#include "power.h"
#include "mbox.h"
#include "cpio.h"

#define CLI_MAX_CMD 8

extern char* _dtb;
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
    if (cli_strcmp(buf, "help") == 0) {
        cmd_help();
    } else if (cli_strcmp(buf, "hello") == 0) {
        cmd_hello();
    } else if (cli_strcmp(buf, "hwinfo") == 0) {
        cmd_hwinfo();
    } else if (cli_strcmp(buf, "reboot") == 0) {
        cmd_reboot();
    } else if(*buf) {
        uart_puts(buf);
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
    uart_puts("   help      - list all commands\r\n");
    uart_puts("   ls        - list all files in directory\r\n");
    uart_puts("   hello     - print hello message\r\n");
    uart_puts("   hwinfo    - print hardware info\r\n");
    uart_puts("   reboot    - reboot the device\r\n");
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
    uart_puts("ls function");

    while(header_ptr != 0) {
        uart_puts("while");

        int err = cpio_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        if (err) {
            uart_puts("CPIO parse error");
        }
        if (header_ptr != 0) {
            uart_puts("hihi"); 

            uart_puts(c_filepath);
            uart_puts("\r\n"); 
        } else {
            uart_puts("nono");
        }
    }
}

void cmd_debug() {
    // This is debug area 
    uart_puts("\r\n");
    uart_puts("===========================================\r\n");
    uart_puts("| * * * * * * * * * * * * * * * * * * * * |\r\n");
    uart_puts("| * * (◍•ᴗ•◍) HAPPY DEBUGGGGG (◍•ᴗ•◍) * * |\r\n");
    uart_puts("| * * * * * * * * * * * * * * * * * * * * |\r\n");
    uart_puts("===========================================\r\n");                    
    uart_puts("\r\n");
    cmd_ls();
    cmd_hello();
}