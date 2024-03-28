#include "headers/command.h"
#include "headers/uart.h"
#include "headers/mailbox.h"
#include "headers/reboot.h"

void help()
{
    display("help:      Print this help munu.\n");
    display("hello:     Print HELLO WORLD!\n");
    display("info:      Print Hardware information.\n");
    display("reboot:    Reboot the device.\n");
}

void hello()
{
    display("HELLO WORLD!\n");
}

void info()
{
    get_board_revision();
    get_arm_memory();
}

void reboot()
{
    display("Rebooting...\n");
    reset(100);
}

void undefined()
{
    display("Command not found.\n");
}