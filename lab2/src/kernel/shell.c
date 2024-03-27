
#include "base.h"
#include "shell.h"
#include "mailbox.h"
#include "reboot.h"
#include "uart.h"
#include "utils.h"
#include "cpio.h"
#include "dtb.h"

extern UPTR* cpio_addr;

int shell_read_cmd(const char* inputStr, const char* cmd) {
	for (int i = 0; i < 32; i++) {
		if (cmd[i] == '\0') {
			return 1;
		}
		if (inputStr[i] != cmd[i])
			return 0;
	}
	return 0;
}

void print_dtb(int token, const char* name, const void* data, U32 size) {
	switch(token){
		case FDT_BEGIN_NODE:
			uart_send_string("\n");
			uart_send_string((char*)name);
			uart_send_string("{\n ");
			break;
		case FDT_END_NODE:
			uart_send_string("\n");
			uart_send_string("}\n");
			break;
		case FDT_NOP:
			break;
		case FDT_PROP:
			uart_send_string((char*)name);
			break;
		case FDT_END:
			break;
	}
}

void shell() {
	
	char array_space[256];
	char* input_str = array_space;

	while (1) {
		char c;
		
		uart_send_string("# ");
		while (1) {
			c = uart_get_char();
			*input_str++ = c;
			if (c == '\n') {
				uart_send_string("\r\n");
				*input_str = '\0';
				break;
			} else {
				uart_send_char(c);
			}
		}
		
		input_str = array_space;
		if (string_compare(input_str, "help")) {
			uart_send_string("help   : print this help menu\n");
			uart_send_string("hello  : print Hello World!\n");
			uart_send_string("info   : show board info\n");
			uart_send_string("reboot : reboot the device\n");
			uart_send_string("ls     : List all CPIO files\n");
			uart_send_string("cat    : print file content\n");
		} else if (string_compare(input_str, "hello")) {
			uart_send_string("Hello World!\n");
		} else if (string_compare(input_str,"info")) {
			get_board_revision();
			if (mailbox_call()) {
				uart_send_string("Revision: ");
    			uart_send_string("0x");
				uart_binary_to_hex(mailbox[5]);
				uart_send_string("\r\n");
			} else {
				uart_send_string("Unable to query revision!\n");
			}
			get_arm_mem();
        	if (mailbox_call()) {
				uart_send_string("Memory base address: ");
    			uart_send_string("0x");
				uart_binary_to_hex(mailbox[5]);
				uart_send_string("\r\n");
				uart_send_string("Memory size: ");
    			uart_send_string("0x");
				uart_binary_to_hex(mailbox[6]);
				uart_send_string("\r\n");
			} else {
				uart_send_string("Unable to query memory info!\n");
			}
			get_serial_number();
        	if (mailbox_call()) {
				uart_send_string("Serial Number: ");
    			uart_send_string("0x");
				uart_binary_to_hex(mailbox[6]);
				uart_binary_to_hex(mailbox[5]);
				uart_send_string("\r\n");
			} else {
				uart_send_string("Unable to query serial!\n");
			}
		} else if (string_compare(input_str,"reboot")) {
			uart_send_string("Rebooting....\n");
			reset(1000);
		}
		 else if (string_compare(input_str, "ls")) {
			cpio_ls(cpio_addr);
		} else if (shell_read_cmd(input_str, "cat")) {
			char fileName[32];
			unsigned int len = 0;
			for(unsigned int i = 0; i < 32; i++) {
				if (input_str[i + 4] == '\0') {
					fileName[i] = '\0';
					len = i - 1;	
					break;
				}
				fileName[i] = input_str[i + 4];
			}
			cpio_cat(cpio_addr, fileName, len);
		} else if (shell_read_cmd(input_str, "dtb")) {
			fdt_traverse(print_dtb);
		}
		 else {
			uart_send_string("Unknown command\n");
		}
	}
}
