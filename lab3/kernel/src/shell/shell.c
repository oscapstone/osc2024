
#include "base.h"
#include "io/uart.h"
#include "io/dtb.h"
#include "io/reboot.h"
//#include "io/mailbox.h"
#include "fs/cpio.h"
#include "utils/utils.h"
#include "utils/printf.h"

extern char* _dtb_ptr;

char* cpio_addr;

void get_cpio_addr(int token, const char* name, const void *data, unsigned int size) {
	if(token == FDT_PROP && (utils_strncmp(name, "linux,initrd-start", 18) == 0)) {
		//uart_send_string("CPIO Prop found!\n");
		U64 dataContent = (U64)data;
		//uart_send_string("Data content: ");
		//uart_hex64(dataContent);
		//uart_send_string("\n");
		U32 cpioAddrBigEndian = *((U32*)dataContent);
		U32 realCPIOAddr = utils_transferEndian(cpioAddrBigEndian);
		//uart_send_string("CPIO address: 0x");
		//uart_hex64(realCPIOAddr);
		//uart_send_string("\n");
		cpio_addr = (char*)realCPIOAddr;
	}
}

void shell() {

    
    char cmd_space[256];

    char* input_ptr = cmd_space;

	// getting CPIO addr
	fdt_traverse(get_cpio_addr);

    while (TRUE) {

        uart_send_nstring(2, "# ");
        
        while (TRUE) {
			while (uart_async_empty()) {
				asm volatile("nop");
			}
            char c = uart_async_get_char();
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
			//uart_send_string("info   : show board info\n");
			uart_send_string("reboot : reboot the device\n");
			uart_send_string("ls     : List all CPIO files\n");
			uart_send_string("cat    : Show file content\n");
			uart_send_string("dtb    : Load DTB data\n");
        } else if (utils_strncmp(cmd_space, "hello", 4) == 0) {
			uart_send_string("Hello World!\n");
        }
		/*else if (utils_strncmp(cmd_space,"info", 4) == 0) {
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
		}*/
		 else if (utils_strncmp(cmd_space, "ls", 2) == 0) {
			cpio_ls();
		} else if (utils_strncmp(cmd_space, "cat ", 4) == 0) {
			char fileName[32];
			U32 len = 0;
			char s;
			U32 iter = 4;
			while ((s = cmd_space[iter]) == ' ') {
				iter++;
			}
			while ((s = cmd_space[iter]) != '\0') {
				fileName[len++] = cmd_space[iter++];
			}
			fileName[len] = '\0';
			len--;
			cpio_cat(fileName, len);
		} else if (utils_strncmp(cmd_space, "dtb", 3) == 0) {
			U64 dtbPtr = (U64) _dtb_ptr;
			uart_send_string("DTB address: ");
			uart_hex64(dtbPtr);
			uart_send_string("\n");
			//fdt_traverse(print_dtb);
			fdt_traverse(get_cpio_addr);
		} else if (utils_strncmp(cmd_space, "reboot", 6) == 0) {
			uart_send_string("Rebooting....\n");
			reset(1000);
		} else if (utils_strncmp(cmd_space, "exception", 9) == 0) {
			printf("\nException level: %d\n", utils_get_el());
		}
		 else {
			uart_send_string("Unknown command\n");
			uart_send_string(cmd_space);
			uart_send_string("\n");
		}

    }

}
