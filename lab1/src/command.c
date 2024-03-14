#include "mini_uart.h"
#include "reboot.h"
#include "mailbox.h"

void cmd_help()
{
    uart_send_string("Usage:\r\n");
    uart_send_string("help\t: print this help menu\r\n");
    uart_send_string("hello\t: print Hello World!\r\n");
    uart_send_string("reboot\t: reboot the device\r\n");
    uart_send_string("cancel reboot\t: cancel reboot the device\r\n");
    uart_send_string("mailbox\t: print Hardware Information\r\n");
}

void cmd_hello()
{
    uart_send_string("Hello World!\r\n");
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
