#include "shell.h"
#include "mini_uart.h"
#include "power.h"

#define SHIFT_ADDR 0x100000

extern char* _dtb;
extern char _start[];

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
    } else if (cli_strcmp(buf, "reboot") == 0) {
        cmd_reboot();
    } else if (cli_strcmp(buf, "load") == 0) {
        cmd_loading();
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
    uart_puts("| * (◍•ᴗ•◍) OSC 2024 BOOTLOADER (◍•ᴗ•◍) * |\r\n");
    uart_puts("| * * * * * * * * * * * * * * * * * * * * |\r\n");
    uart_puts("===========================================\r\n");                    
    uart_puts("\r\n");
}

void cmd_help() {
    uart_puts("Example usage:\r\n");
    uart_puts("   help      - list all commands\r\n");
    uart_puts("   hello     - print hello message\r\n");
    uart_puts("   load      - load new kernel image to rpi3\r\n");
    uart_puts("   reboot    - reboot the device\r\n");
}

void cmd_hello() {
    uart_puts("Hello World!\r\n");
}

void cmd_reboot() {
    uart_puts("Reboot in 5 seconds ...\r\n\r\n");
    volatile unsigned int* rst_addr = (unsigned int*)PM_RSTC;
    *rst_addr = PM_PASSWORD | 0x20;
    volatile unsigned int* wdg_addr = (unsigned int*)PM_WDOG;
    *wdg_addr = PM_PASSWORD | 5;
}

void cmd_loading() {
    char* bak_dtb = _dtb;
    char c;
    unsigned long long kernel_size = 0;
    char* kernel_start = (char*) (&_start);
    uart_puts("Please upload the image file.\r\n");
    for (int i=0; i<8; i++) {
        c = uart_getc();
        kernel_size += c<<(i*8);
    }
    for (int i=0; i<kernel_size; i++) {
        c = uart_getc();
        kernel_start[i] = c;
    }
    uart_puts("Image file downloaded successfully.\r\n");
    uart_puts("Point to new kernel ...\r\n");

    ((void (*)(char*))kernel_start)(bak_dtb);
}
