#include "shell.h"
#include "mini_uart.h"
#include "reboot.h"
#include "mail.h"
#include "helper.h"
#include "loader.h"
#include "cpio.h"
#include "alloc.h"
#include "fdt.h"
#include "timer.h"
#include "exception.h"

char buf[1024];

void help() {
	uart_printf("help         : print this help menu\r\n");
	uart_printf("hello        : print Hello World!\r\n");
	uart_printf("reboot       : reboot the device\r\n");
	uart_printf("revision     : get the revision number\r\n");
	uart_printf("memory       : get the ARM memory info\r\n");
	uart_printf("ls           : ls the initial ramdisk\r\n");
	uart_printf("cat          : cat the initial ramdisk\r\n");
	uart_printf("initramd     : use devicetree to get initial ramdisk\r\n");
	uart_printf("dtb          : output the device tree\r\n");
	uart_printf("async        : test for async uart\r\n");
	uart_printf("timeout      : set timeout (ms)\r\n");
}

void handle_timeout() {
	uart_printf("Input time(ms): ");
	uart_recv_string(buf);
	unsigned long t = stoi(buf);
	uart_printf("\n");
	uart_printf("Input data to output: ");
	uart_recv_string(buf);
	uart_printf("\n");
	set_timeout(t, buf);
}

void async_test() {
	uart_irq_on();

	uart_irq_send("Test\r\n\0");

	int t = 100000000;
	while (t --); 

	char* str = simple_malloc(100);
	uart_irq_read(str);

	t = 10000000;
	while (t --);
	
	uart_irq_off();
	uart_printf("%s, End\n", str);
}

void shell_begin(char* fdt)
{
	while (1) {
		uart_printf("# ");
		uart_recv_string(buf);
		uart_printf("\r\n");
		if (same(buf, "hello")) {
			uart_printf("Hello World!\n");
		}
		else if (same(buf, "tt")) {
			uart_printf("Input:\r\n");
			uart_recv_string(buf);
			uart_printf("\r\n");
			unsigned long t = stoi(buf);
			uart_printf("started countind from %d\r\n", t);
			while (t--) {
				
			}
			uart_printf("end\r\n");
		}
		else if (same(buf, "timeout")) {
			handle_timeout();
		}
		else if (same(buf, "help")) {
			help();
		}
		else if (same(buf, "async")) {
			async_test();
		}
		else if(same(buf, "initramd")) {
			fdt_traverse(get_initramfs_addr, fdt);
		}
		else if (same(buf, "ls")) {
			cpio_ls();
		}
		else if (same(buf, "cat")) {
			uart_printf("Filename: ");
			uart_recv_string(buf);
			uart_printf("\n");
			cpio_cat(buf);
		}
		else if (same(buf, "dtb")) {
			fdt_traverse(print_dtb, fdt);
		}
		else if(same(buf, "revision")) {
			unsigned int rev = get_board_revision();
			if (rev) {
				uart_printf("%x", rev);
			}
			else {
				uart_printf("Failed to get board revision\n");
			}
		}
		else if(same(buf, "memory")) {
			unsigned int arr[2];
			arr[0] = arr[1] = 0;
			if(!get_arm_memory(arr)) {
				uart_printf("Your ARM memory address base is: %x", arr[0]);
				uart_printf("Your ARM memory size is: %x", arr[1]);
			}
			else {
				uart_printf("Failed getting ARM memory info\n");
			}
		}
		else if (same(buf, "reboot")) {
			uart_printf("Rebooting\n");
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
		else if (same(buf, "load")) {
			uart_printf("Filename: ");
			uart_recv_string(buf);
			uart_printf("\n");
			cpio_load(buf);
		}
		else {
			uart_printf("Command not found\n");
			help();
		}
	}
}
