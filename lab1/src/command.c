#include "uart.h"
#include "mailbox.h"
#include "string.h"
#include "command.h"

void command_not_found() {
    uart_puts("ERR: command not found.\n\n");
    command_help();
}

void input_buffer_overflow_message( char cmd[] ) {
    uart_puts("Follow command: \"");
    uart_puts(cmd);
    uart_puts("\"... is too long to process.\n");

    uart_puts("The maximum length of input is 64.\n\n");
}

void command_help() {
    uart_puts("help\t: print this help menu\n");
    uart_puts("hello\t: print Hello World!\n");
    uart_puts("reboot\t: reboot the device\n");
    uart_puts("\n");
}

void command_hello() {
    uart_puts("Hello, world!\n\n");
}

void set(long addr, unsigned int value) {
    volatile unsigned int* point = (unsigned int*)addr;
    *point = value;
}

void reset(int tick) {                 // reboot after watchdog timer expire
    set(PM_RSTC, PM_PASSWORD | 0x20);  // full reset
    set(PM_WDOG, PM_PASSWORD | tick);  // number of watchdog tick
}

void command_cancel_reset() {
    uart_puts("Rebooting Attempt Aborted, if any.\n\n");
    set(PM_RSTC, PM_PASSWORD | 0); 
    set(PM_WDOG, PM_PASSWORD | 0); 
}

void command_reboot() {
    uart_puts("Start Rebooting...\n\n");
    reset(100000);
}

void command_info() {
    char str[20];  
    uint32_t board_revision = mbox_get_board_revision();
    uint32_t arm_mem_base_addr;
    uint32_t arm_mem_size;

    mbox_get_arm_memory_info(&arm_mem_base_addr, &arm_mem_size);
    
    uart_puts("Board Revision: ");
    if ( board_revision ) {
        itohex_str(board_revision, sizeof(uint32_t), str);
        uart_puts(str);
        uart_puts("\n");
    } else {
        uart_puts("Unable to query serial!\n");
    }
    uart_puts("ARM memory base address: ");
    if ( arm_mem_base_addr ) {
        itohex_str(arm_mem_base_addr, sizeof(uint32_t), str);
        uart_puts(str);
        uart_puts("\n");
    } else {
        uart_puts("0x00000000\n");
    }
    uart_puts("ARM memory size: ");
    if ( arm_mem_size ) {
        itohex_str( arm_mem_size, sizeof(uint32_t), str);
        uart_puts(str);
        uart_puts("\n\n");
    } else {
        uart_puts("0x00000000\n\n");
    }
}

void command_clear() {
    uart_puts("\033[2J\033[H");
}