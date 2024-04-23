
#include "base.h"
#include "io/uart.h"
#include "io/dtb.h"
#include "io/reboot.h"
#include "io/mailbox.h"
#include "utils/utils.h"

extern char* _dtb_ptr;
extern char* cpio_addr;

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
			uart_send_string("info   : show board info\n");
			uart_send_string("reboot : reboot the device\n");
			uart_send_string("ls     : List all CPIO files\n");
			uart_send_string("cat    : Show file content\n");
			uart_send_string("dtb    : Load DTB data\n");
        } else if (utils_strncmp(cmd_space, "hello", 4) == 0) {
			uart_send_string("Hello World!\n");
        }else if (utils_strncmp(cmd_space,"info", 4) == 0) {
			get_board_revision();
			if (mailbox_call()) {
				uart_send_string("Revision: ");
    			uart_send_string("0x");
				uart_binary_to_hex(mailbox[5]);
				uart_send_string("\n");
			} else {
				uart_send_string("Unable to query revision!\n");
			}
			get_arm_mem();
        	if (mailbox_call()) {
				uart_send_string("Memory base address: ");
    			uart_send_string("0x");
				uart_binary_to_hex(mailbox[5]);
				uart_send_string("\n");
				uart_send_string("Memory size: ");
    			uart_send_string("0x");
				uart_binary_to_hex(mailbox[6]);
				uart_send_string("\n");
			} else {
				uart_send_string("Unable to query memory info!\n");
			}
			get_serial_number();
        	if (mailbox_call()) {
				uart_send_string("Serial Number: ");
    			uart_send_string("0x");
				uart_binary_to_hex(mailbox[6]);
				uart_binary_to_hex(mailbox[5]);
				uart_send_string("\n");
			} else {
				uart_send_string("Unable to query serial!\n");
			}
		} else if (utils_strncmp(cmd_space, "ls", 2) == 0) {
			cpio_ls();
		} else if (utils_strncmp(cmd_space, "cat", 3) == 0) {
			char fileName[32];
			unsigned int len = 0;
			for(unsigned int i = 0; i < 32; i++) {
				if (cmd_space[i + 4] == '\0') {
					fileName[i] = '\0';
					len = i - 1;	
					break;
				}
				fileName[i] = cmd_space[i + 4];
			}
			cpio_cat(fileName, len);
		} else if (utils_strncmp(cmd_space, "dtb", 3) == 0) {
			U64 dtbPtr = (U64) _dtb_ptr;
			uart_send_string("DTB address: ");
			uart_hex64(dtbPtr);
			uart_send_string("\n");
			//fdt_traverse(print_dtb);
			//fdt_traverse(get_cpio_addr);
		} else if (utils_strncmp(cmd_space, "reboot", 6) == 0) {
			uart_send_string("Rebooting....\n");
			reset(1000);
		}
		 else {
			uart_send_string("Unknown command\n");
			uart_send_string(cmd_space);
			uart_send_string("\n");
		}

    }

}
