#include "shell.h"
#include "mini_uart.h"
#include "reboot.h"
#include "mail.h"
#include "helper.h"
#include "loader.h"

char buf[1024];

void help() {
	output("help      : print this help menu");
	output("hello     : print Hello World!");
	output("reboot    : reboot the device");
	output("revision  : get the revision number");
	output("memory    : get the ARM memory info");
}

void shell_begin(void)
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
		else if (same(buf, "load kernel")) {
			load_kernel();
			// never comes here :(
		}
		else if (same(buf, "reboot")) {
			output("Rebooting");
			reset(100);
		}
		else if (same(buf, "exit")) {
			break;
		}
		else {
			output("Command not found");
			help();
		}
	}
}
