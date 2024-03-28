#include "shell.h"
#include "mini_uart.h"
#include "reboot.h"
#include "mail.h"
#include "helper.h"
#include "loader.h"
#include "cpio.h"
#include "alloc.h"
#include "fdt.h"

char buf[1024];

void help() {
	output("help         : print this help menu");
	output("hello        : print Hello World!");
	output("reboot       : reboot the device");
	output("revision     : get the revision number");
	output("memory       : get the ARM memory info");
	output("ls           : ls the initial ramdisk");
	output("cat          : cat the initial ramdisk");
	output("test alloc   : test the allocator");
	output("get initramd : use devicetree to get initial ramdisk");
	output("dtb          : output the device tree");
	
}

void shell_begin(char* fdt)
{
	while (1) {
		uart_send_string("# ");
		uart_recv_string(buf);
		uart_send_string("\r\n");
		if (same(buf, "hello")) {
			output("Hello World!");
		}
		else if (same(buf, "help")) {
			help();
		}
		else if(same(buf, "get initramd")) {
			fdt_traverse(get_initramfs_addr, fdt);
		}
		else if (same(buf, "ls")) {
			parse_cpio_ls();
		}
		else if (same(buf, "cat")) {
			uart_send_string("Filename: ");
			uart_recv_string(buf);
			output("");
			parse_cpio_cat(buf);
		}
		else if (same(buf, "dtb")) {
			fdt_traverse(print_dtb, fdt);
		}
		else if(same(buf, "revision")) {
			unsigned int rev = get_board_revision();
			if (rev) {
				output_hex(rev);
			}
			else {
				output("failed to do get board revision");
			}
		}
		else if(same(buf, "memory")) {
			unsigned int arr[2];
			arr[0] = arr[1] = 0;
			if(!get_arm_memory(arr)) {
				uart_send_string("Your ARM memory address base is: ");
				output_hex(arr[0]);
				uart_send_string("Your ARM memory size is: ");
				output_hex(arr[1]);
			}
			else {
				output("You failed getting ARM memory info");
			}
		}
		else if (same(buf, "reboot")) {
			output("Rebooting");
			reset(100);
		}
		else if (same(buf, "exit")) {
			break;
		}
		else if (same(buf, "test alloc")) {
			char* str = simple_malloc(8);
			char* str2 = simple_malloc(8);
			for(int i = 0; i < 8; i ++) {
				str[i] = 'a';
				str2[i] = 'b';
			}
			uart_printf("%x, %x\n", str, str2);
		}
		else {
			output("Command not found");
			help();
		}
	}
}
