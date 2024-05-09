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
	char* buf = my_malloc(512);
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

	char* str = simple_malloc(100, sizeof(char));
	uart_irq_read(str);

	t = 10000000;
	while (t --);
	
	uart_irq_off();
	uart_printf("%s, End\n", str);
}

void page_test() {
	print_node_list();
	int arr0 = my_malloc(4096 * 1);
	int arr1 = my_malloc(4096 * 2);
	int arr2 = my_malloc(4096 * 2);
	int arr3 = my_malloc(4096 * 4);
	int arr4 = my_malloc(4096 * 8);
	print_node_list();
	int arr5 = my_malloc(4096 * 1);
	uart_printf ("%d, %d, %d, %d, %d, %d\r\n", arr0 >> 12, arr1 >> 12, arr2 >> 12, 
	arr3 >> 12, arr4 >> 12, arr5 >> 12);
	print_node_list();
	my_free(arr5);
	print_node_list();
}

void malloc_test() {
	print_node_list();
	char* arr[17];
	for (int i = 0; i < 17; i ++) {
		arr[i] = my_malloc(512);
		uart_printf("%d\r\n", arr[i]);
	}
	print_node_list();
	print_pool();
	for (int i = 0; i < 17; i ++) {
		my_free(arr[i]);
	}
	print_node_list();
	print_pool();
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
			cpio_parse_ls();
		}
		else if (same(buf, "cat")) {
			uart_printf("Filename: ");
			uart_recv_string(buf);
			uart_printf("\n");
			cpio_parse_cat(buf);
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
		else if (same(buf, "page")) {
			page_test();
		}
		else if (same(buf, "malloc")) {
			malloc_test();
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
