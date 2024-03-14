#include "header/utils.h"
#include "header/uart.h"
#include "header/shell.h"
#include "header/reboot.h"
#include "header/mailbox.h"

int main(){
    uart_init();
    uart_send_char('\n');
    char *s = "Type in `help` to get instruction menu!\n";
    uart_send_str(s);
    shell();
    return 0;
}