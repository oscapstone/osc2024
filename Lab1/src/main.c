#include "uart.h"
#include "shell.h"
#include "mailbox.h"

int main()
{
    uart_init();

    print_hd_info();

    shell_main();

    return 0;
}
