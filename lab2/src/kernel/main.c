
#include "base.h"
#include "dtb.h"
#include "uart.h"
#include "utils.h"
#include "shell.h"

UPTR cpio_addr;

void initramfs_callback(int token, const char* name, const void* data, U32 size) {
	if(token==FDT_PROP && utils_strncmp((char *)name,"linux,initrd-start", 18)) {
		UPTR cpioPtr = (UPTR)utils_transferEndian(data);
		cpio_addr = cpioPtr;
		uart_send_string("cpio address is at: ");
		uart_hex(cpioPtr);
		uart_send_char('\n');
	}
}

void main() {
	
	uart_init();

	cpio_addr = 0x8000000;

	fdt_traverse(initramfs_callback);
	
	shell();
	
	return;
}
