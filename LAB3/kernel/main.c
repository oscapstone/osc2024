#include "mini_uart.h"
#include "dtb.h"
#include "exception_c.h"
#include "utils_s.h"
#include "utils_c.h"
#include "shell.h"
#include "timer.h"

extern void *_dtb_ptr;

void kernel_main(void)
{
    // uart_init();
    timeout_event_init();
    uart_send_string("Hello, world!\n");
    fdt_traverse(get_initramfs_addr, _dtb_ptr);

    // To print out el number
    int el = get_el_TYPE();
    char el_int[2];
    utils_int2str_dec(el, el_int);
    char tmp[100];
    cat_two_strings("kernel Exception level: ", el_int, tmp);
    mini_printf("%s\n", tmp);

    enable_interrupt();
    shell();
}