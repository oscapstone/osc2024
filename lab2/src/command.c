#include "mini_uart.h"
#include "reboot.h"
#include "mailbox.h"
#include "init_ramdisk.h"
#include "alloc.h"

void cmd_help()
{
    uart_send_string("Usage:\r\n");
    uart_send_string("help\t\t: print this help menu\r\n");
    uart_send_string("hello\t\t: print Hello World!\r\n");
    uart_send_string("ls\t\t: list files in initramfs.cpio\r\n");
    uart_send_string("cat\t\t: print the content of an arbitrary file\r\n");
    uart_send_string("reboot\t\t: reboot the device\r\n");
    uart_send_string("cancel reboot\t: cancel reboot the device\r\n");
    uart_send_string("mailbox\t\t: print Hardware Information\r\n");
}

void cmd_hello()
{
    uart_send_string("Hello World!\r\n");
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
    uart_send_string("Rebooting...\r\n");
    reset(1000);
}

void cmd_cancel_reboot()
{
    uart_send_string("Cancel reboot!\r\n");
    cancel_reset();
}

void cmd_mailbox()
{
    hardware_board_revision();
    hardware_vc_memory();
}

void cmd_not_found()
{
    uart_send_string("shell: command not found\r\n");
}
