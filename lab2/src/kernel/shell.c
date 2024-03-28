#include "base.h"
#include "shell.h"
#include "mailbox.h"
#include "reboot.h"
#include "uart.h"
#include "utils.h"
#include "cpio.h"
#include "dtb.h"

extern void* _dtb_ptr;

void print_dtb(int type,const char* name,const void *data,unsigned int size) {

}

char* cpio_addr;

void get_cpio_addr(int token, const char* name, const void *data, unsigned int size) {
	if(token == FDT_PROP && (utils_strncmp(name, "linux,initrd-start", 18) == 0)) {
		uart_send_string("CPIO Prop found!\n");
		U64 dataContent = (U64)data;
		uart_send_string("Data content: ");
		uart_hex64(dataContent);
		uart_send_string("\n");
		U32 cpioAddrBigEndian = *((U32*)dataContent);
		U32 realCPIOAddr = utils_transferEndian(cpioAddrBigEndian);
		uart_send_string("CPIO address: 0x");
		uart_hex64(realCPIOAddr);
		uart_send_string("\n");
		cpio_addr = (char*)realCPIOAddr;
	}
}

void shell() {
	
	char array_space[256];
	char* input_str = array_space;
	
	//cpio_addr = (char*)0x8000000;
	//char* cpio_addr = (char*)0x20000000;

	fdt_traverse(get_cpio_addr);

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
		if (utils_strncmp(input_str, "help", 4) == 0) {
			uart_send_string("help   : print this help menu\n");
			uart_send_string("hello  : print Hello World!\n");
			uart_send_string("info   : show board info\n");
			uart_send_string("reboot : reboot the device\n");
			uart_send_string("ls     : List all CPIO files\n");
			uart_send_string("cat    : Show file content\n");
			uart_send_string("dtb    : Load DTB data\n");
		} else if (utils_strncmp(input_str, "hello", 5) == 0) {
			uart_send_string("Hello World!\n");
		} else if (utils_strncmp(input_str,"info", 4) == 0) {
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
		} else if (utils_strncmp(input_str, "ls", 2) == 0) {
			cpio_ls();
		} else if (utils_strncmp(input_str, "cat", 3) == 0) {
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
			cpio_cat(fileName, len);
		} else if (utils_strncmp(input_str, "dtb", 3) == 0) {
			U64 dtbPtr = (U64) _dtb_ptr;
			uart_send_string("DTB address: ");
			uart_hex64(dtbPtr);
			uart_send_string("\n");
			//fdt_traverse(print_dtb);
			fdt_traverse(get_cpio_addr);
		} else if (utils_strncmp(input_str,"reboot", 6) == 0) {
			uart_send_string("Rebooting....\n");
			reset(1000);
		}
		 else {
			uart_send_string("Unknown command\n");
			uart_send_string(input_str);
			uart_send_string("\n");
		}
	}
}
