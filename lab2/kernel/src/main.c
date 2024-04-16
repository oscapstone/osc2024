#include "uart1.h"
#include "shell.h"
#include "heap.h"
#include "dtb.h"

extern char *dtb_ptr;

void main(char *arg)
{
    dtb_ptr = arg;
    traverse_device_tree(dtb_ptr, dtb_callback_initramfs);

    uart_init();
    uart_flush_FIFO();
    start_shell();
}