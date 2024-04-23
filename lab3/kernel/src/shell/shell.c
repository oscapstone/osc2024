
#include "base.h"
#include "io/uart.h"
#include "utils/utils.h"

void shell() {

    
    char cmd_space[256];

    char* input_ptr = cmd_space;


    while (TRUE) {

        uart_send_nstring(2, "# ");
        
        while (TRUE) {
            char c = uart_get_char();
            *input_ptr++ = c;
            if (c == '\n' || c == '\r') {
                uart_send_nstring(2, "\r\n");
                *input_ptr = '\0';
                break;
            } else {
                uart_send_char(c);
            }
        }

        input_ptr = cmd_space;
        if (utils_strncmp(cmd_space, "help", 4) == 0) {
            uart_send_string("NS shell ver 0.02\n");
            uart_send_string("help   : print this help menu\n");
			uart_send_string("hello  : print Hello World!\n");
        } else if (utils_strncmp(cmd_space, "hello", 4) == 0) {
			uart_send_string("Hello World!\n");
        }

    }

}
