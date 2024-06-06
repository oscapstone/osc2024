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
#include "thread.h"
#include "system_call.h"
#include "signal.h"

char buf[1024];

extern void core_timer_enable();

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

	uart_irq_send("Test\n", 5);

	int t = 100000000;
	while (t --); 

	char* str = my_malloc(512);
	int len;
	for (len = 0; ; len ++) {
		str[len] = uart_irq_read();
		if (str[len] == 0) break;
	}
	
	uart_irq_off();
	uart_printf("%s, End\n", str);
}

void page_test() {
	print_node_list();
	int* arr0 = (int*)my_malloc(4096 * 1);
	int* arr1 = (int*)my_malloc(4096 * 2);
	int* arr2 = (int*)my_malloc(4096 * 2);
	int* arr3 = (int*)my_malloc(4096 * 4);
	int* arr4 = (int*)my_malloc(4096 * 8);
	print_node_list();
	int* arr5 = (int*)my_malloc(4096 * 1);
	for (int i = 0; i < 4096; i += sizeof(int)){
		arr0[i] = 1;
	}
	for (int i = 0; i < 4096 * 2; i += sizeof(int)){
		arr1[i] = 1;
	}
	for (int i = 0; i < 4096 * 2; i += sizeof(int)){
		arr2[i] = 1;
	}
	for (int i = 0; i < 4096 * 4; i += sizeof(int)){
		arr3[i] = 1;
	}
	for (int i = 0; i < 4096 * 8; i += sizeof(int)){
		arr4[i] = 1;
	}
	for (int i = 0; i < 4096; i += sizeof(int)){
		arr5[i] = 1;
	}
	print_node_list();
	my_free(arr5);
	print_node_list();
}

void malloc_test() {
	print_node_list();
	char* arr[17];
	for (int i = 0; i < 17; i ++) {
		arr[i] = my_malloc(512);
		for (int j = 0; j < 512; j ++) {
			arr[i][j] = 'a';
		}
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
				fdt_traverse(get_initramfs_addr);
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
				fdt_traverse(print_dtb);
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
		else if (same(buf, "thread")) {
			thread_test();
		}
		else if (same(buf, "fork")) {
			thread_create(from_el1_to_fork_test);
		}
		else if (same(buf, "timer")) {
			core_timer_enable();
		}
		else {
			uart_printf("Command not found\n");
			help();
		}
	}
}
