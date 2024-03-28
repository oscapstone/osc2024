#include "header/utils.h"
#include "header/uart.h"
#include "header/shell.h"
#include "header/reboot.h"
#include "header/mailbox.h"
#include "header/cpio.h"
#include "header/dtb.h"
extern char *dtb_base;
int main(char *arg){
    uart_init();
    dtb_base = arg;
    fdt_traverse(initramfs_callback);
    uart_send_str("\x1b[2J\x1b[H");
    char *s = "Type in `help` to get instruction menu!\n";
    uart_send_str(s);
    shell();
    return 0;
}