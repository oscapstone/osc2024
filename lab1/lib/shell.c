#include "shell.h"

char buf[0x100];

void welcome_msg() {
    uart_write_string(
        ENDL
        "   ____    _____   _____   ___    ___   ___   _  _" ENDL
        "  / __ \\  / ____| / ____| |__ \\  / _ \\ |__ \\ | || |" ENDL
        " | |  | || (___  | |         ) || | | |   ) || || |_" ENDL
        " | |  | | \\___ \\ | |        / / | | | |  / / |__   _|" ENDL
        " | |__| | ____) || |____   / /_ | |_| | / /_    | | "ENDL
        "  \\____/ |_____/  \\_____| |____| \\___/ |____|   |_|" ENDL);
}

void read_cmd() {
    char tmp;
    uart_write_string("# ");
    for (uint32_t i = 0; tmp = uart_read();) {
        uart_write(tmp);
        switch (tmp) {
            case '\r':
            case '\n':
                buf[i++] = '\0';
                return;
            case 127:  // Backspace
                if (i > 0) {
                    i--;
                    buf[i] = '\0';
                    uart_write_string("\b \b");
                }
                break;
            default:
                buf[i++] = tmp;
                break;
        }
    }
}

void exec_cmd() {
    if (!strlen(buf)) return;
    for (uint32_t i = 0; i < sizeof(func_list) / sizeof(struct func); i++) {
        if (!strcmp(buf, func_list[i].name)) {
            func_list[i].ptr();
            return;
        }
    }
    cmd_unknown();
}

void cmd_help() {
    for (uint32_t i = 0; i < sizeof(func_list) / sizeof(struct func); i++) {
        uart_write_string(func_list[i].name);
        for (uint32_t j = 0; j < (10 - strlen(func_list[i].name)); j++) uart_write(' ');
        uart_write_string(": ");
        uart_write_string(func_list[i].desc);
        uart_write_string(ENDL);
    }
}

void cmd_hello() {
    uart_write_string("Hello World!" ENDL);
}



void cmd_sysinfo() {
    uint32_t *board_revision;
    uint32_t *board_serial_msb, *board_serial_lsb;
    uint32_t *mem_base, *mem_size;
    const int padding = 20;

    // Board Revision
    get_board_revision(board_revision);
    uart_write_string("Board Revision      : 0x");
    uart_puth(*board_revision);
    uart_write_string(ENDL);

    
    // Memory Info
    get_memory_info(mem_base, mem_size);
    uart_write_string("Memroy Base Address : 0x");
    uart_puth(*mem_base);
    uart_write_string(ENDL);

    uart_write_string("Memory Size         : 0x");
    uart_puth(*mem_size);
    uart_write_string(ENDL);
}

void cmd_reboot() {
    uart_write_string("Rebooting...."ENDL);
    reset(10);
}

void cmd_unknown() {
    uart_write_string("Unknown command: ");
    uart_write_string(buf);
    uart_write_string(ENDL);
}

void shell() {
    welcome_msg();
    do {
        read_cmd();

        uart_write_string("# ");
        uart_write_string(buf);
        uart_write_string(ENDL);

        exec_cmd();
    } while (1);
}