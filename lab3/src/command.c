#include "mini_uart.h"
#include "reboot.h"
#include "mailbox.h"
#include "init_ramdisk.h"
#include "alloc.h"
#include "timer.h"

void cmd_help()
{
    uart_async_send_string("Usage:\r\n");
    uart_async_send_string("help\t\t: print this help menu\r\n");
    uart_async_send_string("hello\t\t: print Hello World!\r\n");
    uart_async_send_string("ls\t\t: list files in initramfs.cpio\r\n");
    uart_async_send_string("cat\t\t: print the content of an arbitrary file\r\n");
    uart_async_send_string("exec\t\t: execute an user program\r\n");
    uart_async_send_string("simpleTimer [sec]\t\t: set a simple timer with sec\r\n");
    uart_async_send_string("setTimeout [msg] [sec]\t\t: set timeout with sec and will print message\r\n");
    uart_async_send_string("reboot\t\t: reboot the device\r\n");
    uart_async_send_string("cancel reboot\t: cancel reboot the device\r\n");
    uart_async_send_string("mailbox\t\t: print Hardware Information\r\n");
}

void cmd_hello()
{
    uart_async_send_string("Hello World!\r\n");
}

void cmd_ls()
{
    init_ramdisk_ls();
}

void cmd_cat(const char *filename)
{
    init_ramdisk_cat(filename);
}

void cmd_malloc(int m_size)
{
    simple_malloc(m_size);
}

void cmd_reboot()
{
    uart_async_send_string("Rebooting...\r\n");
    reset(1000);
}

void cmd_cancel_reboot()
{
    uart_async_send_string("Cancel reboot!\r\n");
    cancel_reset();
}

void cmd_mailbox()
{
    hardware_board_revision();
    hardware_vc_memory();
}

void cmd_exec(const char *filename)
{
    init_ramdisk_load_user_prog(filename);
}

void cmd_simple_timer(unsigned int timeout, char *seconds)
{
    // uart_async_send_string("Triggering timer interrupt...\n");
    unsigned long long current_time, cntfrq;
    // read current count of the timer
    asm volatile("mrs %0, cntpct_el0" : "=r"(current_time)); 
    // read the frequency of the counter
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq));
    // convert the current time to seconds
    unsigned int sec_since_boot = current_time / cntfrq;
    
    // uart_async_send_string("Current time: ");
    // uart_send_string_int2hex(sec_since_boot);
    // uart_async_send_string(" seconds\r\n");
    uart_async_send_string("Setting up a timer interrupt after ");
    uart_async_send_string(seconds);
    uart_async_send_string(" seconds.\r\n");
    
    // cntp_ctl_el0: Timer control register. Bit 0 is the enable bit. Set to 1 to enable the timer.
    // cntp_tval_el0: Count timer count from the current timer count and interrupt the CPU core when it reaches the value.
    // cntpct_el0: current count of the timer
    // cntfrq_el0: the frequency of the counter
    asm volatile(
        "mov x20, 1;"
        "msr cntp_ctl_el0, x20;"
        "mrs x20, cntfrq_el0;"
        "mul x20, x20, %0;" 
        "msr cntp_tval_el0, x20;"
        "mov x20, 2;"
        "ldr x1, =0x40000040;"
        "str w20, [x1];"
        :
        : "r"(timeout) 
    );
}

void cmd_timeout(unsigned int timeout, char *message, char *seconds)
{
    uart_async_send_string(message);
    uart_async_send_string(" will be printed in ");
    uart_async_send_string(seconds);
    uart_async_send_string(" seconds\n");

    set_timeout(message, timeout);
}

void cmd_not_found()
{
    uart_async_send_string("shell: command not found\r\n");
}
