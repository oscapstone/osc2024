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
#include "thread.h"
#include "mmu.h"

char buf[1024];
extern thread* get_current();
extern thread** threads;
extern void** thread_fn;

extern char _code_start[];

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

void do_simple_fork_test() {
	irq(0);
	int id = thread_create(from_el1_to_fork_test);
	thread* child = threads[id];
	thread* cur = get_current();
	int t = (cur -> code_size + 4096 - 1) / 4096 * 4096;
	child -> code = my_malloc(t);
	child -> code_size = cur -> code_size;

	uart_printf ("Child code physical: %llx\r\n", child -> code);

	uart_printf ("simple fork test: ");
	for (int j = 0; j < t / 4096; j ++) {
		for (int i = 0; i < 10; i ++) {
			uart_printf ("%d ", (_code_start + 4096 * j)[i]);
		}
		uart_printf ("\r\n");
	}

	uart_printf ("copying code of size %d\r\n", t);
	strcpy(cur -> code, child -> code, t);
	uart_printf ("fuck: ");	
	for (int i = 0; i < 10; i ++) {
		uart_printf ("%d ", ((char*)(child -> code + 4096))[i]);
	}
	uart_printf ("\r\n");
	uart_printf ("t / 4096 : %d\r\n", t / 4096);
	for (int i = 0; i < t / 4096; i ++) {
		map_page(pa2va(child -> PGD), i * 4096, va2pa(child -> code) + i * 4096, (1 << 6));
	}

	for (int i = 0; i < 4; i ++) {
		map_page(pa2va(child -> PGD), 0xffffffffb000L + i * 4096, va2pa(child -> stack_start) + i * 4096, (1 << 6));
	}

	setup_peripheral_identity(pa2va(child -> PGD));

	child -> code = 0;
	child -> stack_start = 0xffffffffb000;

	uart_printf ("%llx\r\n", child -> PGD);

	uart_printf ("thread_fn[%d]: %llx\r\n", id, thread_fn[id]);

	irq(1);
}

void shell_begin(char* fdt)
{
	fdt += 0xffff000000000000;
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
			do_simple_fork_test();
			while (1);
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
