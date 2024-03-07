#include "alloc.h"
#include "cpio_.h"
#include "fb.h"
#include "mbox.h"
#include "my_string.h"
#include "uart0.h"
#include "utli.h"

enum ANSI_ESC {
    Unknown,
    CursorForward,
    CursorBackward,
    Delete
};

enum ANSI_ESC decode_csi_key() {
    char c = uart_read();
    if (c == 'C') {
        return CursorForward;
    } else if (c == 'D') {
        return CursorBackward;
    } else if (c == '3') {
        c = uart_read();
        if (c == '~') {
            return Delete;
        }
    }
    return Unknown;
}

enum ANSI_ESC decode_ansi_escape() {
    char c = uart_read();
    if (c == '[') {
        return decode_csi_key();
    }
    return Unknown;
}

void shell_init() {
    uart_init();
    uart_flush();
    uart_printf("\n[%f] Init PL011 UART done\n", get_timestamp());
}

void shell_input(char *cmd) {
    uart_printf("\r# ");
    int idx = 0, end = 0, i;
    cmd[0] = '\0';
    char c;
    while ((c = uart_read()) != '\n') {
        if (c == 27) { // Decode CSI key sequences
            enum ANSI_ESC key = decode_ansi_escape();
            switch (key) {
            case CursorForward:
                if (idx < end)
                    idx++;
                break;
            case CursorBackward:
                if (idx > 0)
                    idx--;
                break;
            case Delete:
                for (i = idx; i < end; i++) { // left shift command
                    cmd[i] = cmd[i + 1];
                }
                cmd[--end] = '\0';
                break;
            case Unknown:
                uart_flush();
                break;
            }
        } else if (c == 3) { // CTRL-C
            cmd[0] = '\0';
            break;
        } else if (c == 8 || c == 127) { // Backspace
            if (idx > 0) {
                idx--;
                for (i = idx; i < end; i++) { // left shift command
                    cmd[i] = cmd[i + 1];
                }
                cmd[--end] = '\0';
            }
        } else {
            if (idx < end) { // right shift command
                for (i = end; i > idx; i--) {
                    cmd[i] = cmd[i - 1];
                }
            }
            cmd[idx++] = c;
            cmd[++end] = '\0';
        }
        uart_printf("\r# %s \r\e[%dC", cmd, idx + 2);
    }

    uart_printf("\n");
}

void shell_controller(char *cmd) {
    if (!strcmp(cmd, "")) {
        return;
    } else if (!strcmp(cmd, "help")) {
        uart_printf("help: print this help menu\n");
        uart_printf("hello: print Hello World!\n");
        uart_printf("ls: list the filenames in cpio archive\n");
        uart_printf("cat: display the content of the speficied file included in cpio archive\n");
        uart_printf("malloc: get a continuous memory space\n");
        uart_printf("timestamp: get current timestamp\n");
        uart_printf("reboot: reboot the device\n");
        uart_printf("poweroff: turn off the device\n");
        uart_printf("brn: get rpi3’s board revision number\n");
        uart_printf("bsn: get rpi3’s board serial number\n");
        uart_printf("arm_mem: get ARM memory base address and size\n");
        uart_printf("showpic: Show a picture\n");
        uart_printf("loadimg: reupload the kernel image if the bootloader is used\n");
    } else if (!strcmp(cmd, "hello")) {
        uart_printf("Hello World!\r\n");
    } else if (!strcmp(cmd, "ls")) {
        cpio_ls();
    } else if (!strncmp(cmd, "cat", 3)) {
        cpio_cat(cmd + 4);
    } else if (!strcmp(cmd, "malloc")) {
        char *m1 = (char *)simple_malloc(8);
        if (!m1) {
            uart_printf("memory allocation fail!\n");
            return;
        }
        sprintf(m1, "12345678");
        uart_printf("%s\n", m1);

        char *m2 = (char *)simple_malloc(8);
        if (!m2) {
            uart_printf("memory allocation fail!\n");
            return;
        }
        sprintf(m2, "98765432");
        uart_printf("%s\n", m2);

        char *m3 = (char *)simple_malloc(8192);
        if (!m3) {
            uart_printf("memory allocation fail!\n");
            return;
        }

    } else if (!strcmp(cmd, "reboot")) {
        uart_printf("Rebooting...\r\n");
        reset();
        while (1)
            ; // hang until reboot
    } else if (!strcmp(cmd, "poweroff")) {
        uart_printf("Shutdown the board...\r\n");
        power_off();
    } else if (!strcmp(cmd, "brn")) {
        get_board_revision();
    } else if (!strcmp(cmd, "bsn")) {
        get_board_serial();
    } else if (!strcmp(cmd, "arm_mem")) {
        get_arm_base_memory_sz();
    } else if (!strcmp(cmd, "showpic")) {
        lfb_showpicture();
    } else if (!strcmp(cmd, "timestamp")) {
        uart_printf("%f\n", get_timestamp());
    } else if (!strcmp(cmd, "loadimg")) {
        asm volatile(
            "ldr x30, =0x60200;"
            "ret;");
    } else {
        uart_printf("shell: command not found: %s\n", cmd);
    }
}
