#include "uart.h"
#include "shell.h"
#include "mailbox.h"
#include "dtb.h"

int main()
{
    uart_init();

    traverse_device_tree(dtb_callback_initramfs);

    shell_main();

    return 0;
}
