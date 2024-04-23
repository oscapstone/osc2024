
#include "base.h"
#include "io/dtb.h"
#include "io/uart.h"
#include "shell/shell.h"
#include "utils/utils.h"

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

void main() {

    // initialze UART
    uart_init();

    // get the cpio address from dtb
    fdt_traverse(get_cpio_addr);

    shell();

}
