#include "mystring.h"
#include "uart.h"
#include "util.h"
#include "mbox.h"
#include "initramfs.h"
#include <stdint.h>
enum ANSI_ESC {
    Unknown,
    CursorForward,
    CursorBackward,
    Delete
};

enum ANSI_ESC decode_csi_key() {
    char c = uart_recv();
    if (c == 'C') {
        return CursorForward;
    }
    else if (c == 'D') {
        return CursorBackward;
    }
    else if (c == '3') {
        c = uart_recv();
        if (c == '~') {
            return Delete;
        }
    }
    return Unknown;
}

enum ANSI_ESC decode_ansi_escape() {
    char c = uart_recv();
    if (c == '[') {
        return decode_csi_key();
    }
    return Unknown;
}


void shell_init(){
    uart_init();
    uart_flush();
    uart_puts("\n\nWelcome to RPI3\n");
}

void shell_input(char* cmd) {
    uart_puts("\r# ");

    int idx = 0, end = 0, i;
    cmd[0] = '\0';
    char c;
    while ((c = uart_recv()) != '\n') {
        // Decode CSI key sequences
        if (c == 27) {
            enum ANSI_ESC key = decode_ansi_escape();
            switch (key) {
                case CursorForward:
                    if (idx < end) idx++;
                    break;

                case CursorBackward:
                    if (idx > 0) idx--;
                    break;

                case Delete:
                    // left shift command
                    for (i = idx; i < end; i++) {
                        cmd[i] = cmd[i + 1];
                    }
                    cmd[--end] = '\0';
                    break;

                case Unknown:
                    uart_flush();
                    break;
            }
        }
        // CTRL-C
        else if (c == 3) {
            cmd[0] = '\0';
            break;
        }
        // Backspace
        else if (c == 8 || c == 127) {
            if (idx > 0) {
                idx--;
                // left shift command
                for (i = idx; i < end; i++) {
                    cmd[i] = cmd[i + 1];
                }
                cmd[--end] = '\0';
            }
        }
        else {
            // right shift command
            if (idx < end) {
                for (i = end; i > idx; i--) {
                    cmd[i] = cmd[i - 1];
                }
            }
            cmd[idx++] = c;
            cmd[++end] = '\0';
        }
        uart_printf("\r# %s \r\e[%dC", cmd, idx + 2);
    }

    uart_puts("\n");
}
void shell_controller(char* cmd, void *dtb_location) {
    if (!strcmp(cmd, "")) {
        return;
    }
    else if (!strcmp(cmd, "help")) {
        uart_puts("help: print all available commands\n");
        uart_puts("hello: print Hello World!\n");
        uart_printf("info: get board info\n");
        uart_printf("reboot: reboot the device\n");
        uart_printf("ls: Print cpio file list.\n");
        uart_printf("cat {filename}: Print content in {filename} \n");
    }
    else if (!strcmp(cmd, "hello")) {
        uart_puts("Hello World!\n");
    }
    else if (!strcmp(cmd, "info")) {
        mbox_board_revision();
        mbox_arm_memory();
    }
    else if (!strcmp(cmd, "reboot")) {
        uart_puts("Rebooting...");
        reset();
        while (1); // hang until reboot
    }
    else if (!strcmpl(cmd, "cat", 3)) {
        unsigned long fileSize;
        char *result = cpio_get_file((void *)initrd_getLo(), cmd+4, &fileSize);
        if (result != NULL) {
            for (int i = 0;i < fileSize;i++) {
                uart_printf("%c", result[i]);
            }
            uart_printf("\n");
        } else {
            uart_printf("'%s' file  not exist!\n", cmd+4);
        }
    }
    else if (!strcmp(cmd, "ls")) {
        cpio_ls((void *)initrd_getLo());
    }
    else {
        uart_printf("shell: command not found: %s\n", cmd);
    }
}