#include "shell.h"
#include "mini_uart.h"
#include "mbox.h"
#include "power.h"

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
            uart_send_string("Exceed the command length limit.");
            break;
        }
        c = uart_recv();
        if (c == '\r') {
            uart_send_string("\r\n");
            break;
        }
        if ((int)c == 127) {  
            if (idx > 0) {
                uart_send_string("\b \b");
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
    } else {
        uart_send_string("sh: Command not found QQ\r\n");
    }
}

void cli_clear_cmd(char* buf, int length) {
    for (int i=0; i<length; i++) {
        buf[i] = '\0';
    }
}

void cli_print_welcome_msg() {
    uart_send_string("\r\n");
    uart_send_string(" _______  _______  ______   ___     ___      _______  _______  ____ \r\n");
    uart_send_string("|       ||       ||      | |   |   |   |    |   _   ||  _    ||    |\r\n");
    uart_send_string("|   _   ||  _____||  _    ||   |   |   |    |  |_|  || |_|   | |   |\r\n");
    uart_send_string("|  | |  || |_____ | | |   ||   |   |   |    |       ||       | |   |\r\n");
    uart_send_string("|  |_|  ||_____  || |_|   ||   |   |   |___ |       ||  _   |  |   |\r\n");
    uart_send_string("|       | _____| ||       ||   |   |       ||   _   || |_|   | |   |\r\n");
    uart_send_string("|_______||_______||______| |___|   |_______||__| |__||_______| |___|\r\n");
    uart_send_string("\r\n");
}

void cmd_help() {
    uart_send_string("Example usage:\r\n");
    uart_send_string("   help      - list all commands\r\n");
    uart_send_string("   hello     - print hello message\r\n");
    uart_send_string("   hwinfo    - print hardware information\r\n");
    uart_send_string("   reboot    - reboot the device\r\n");
}

void cmd_hello() {
    uart_send_string("Hello World!\r\n");
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
        uart_send_string("Hardware Revision\t: ");
        uart_2hex(pt[6]);
        uart_2hex(pt[5]);
        uart_send_string("\r\n");
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
        uart_send_string("ARM Memory Base Address\t: ");
        uart_2hex(pt[5]);
        uart_send_string("\r\n");
        uart_send_string("ARM Memory Size\t\t: ");
        uart_2hex(pt[6]);
        uart_send_string("\r\n");
    }
}

void cmd_reboot() {
    uart_send_string("Reboot in 5 seconds ...\r\n\r\n");
    volatile unsigned int* rst_addr = (unsigned int*)PM_RSTC;
    *rst_addr = PM_PASSWORD | 0x20;
    volatile unsigned int* wdg_addr = (unsigned int*)PM_WDOG;
    *wdg_addr = PM_PASSWORD | 5;
}
