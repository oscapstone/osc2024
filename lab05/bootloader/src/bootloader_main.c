#include "mini_uart.h"
#include "io.h"
#include "bootload.h"

/*
0x60000: bootloader start address

0x!!!!!: load_kernel() -> 0x????? - 0x20000
***** need to branch hereh *****  -> 0x????? - 0x20000 + 4

			.
			.
			.

0x80000: kernel start address

0x?????: load_kernel()
*/

void readcmd(char*);

void *dtb_addr = 0; // Due to the variable is used in the assembly code, must be declared as global variable


void main()
{
    uart_init();

	while(1)
	{
		printf("\nType load for bootloading: ");
		char cmd[10];
		readcmd(cmd);
		if(cmd[0] == 'l' && cmd[1] == 'o' && cmd[2] == 'a' && cmd[3] == 'd')
		{
			break;
		}
	}

	reallocate();
	asm volatile("b -131068"); // jump to same place in bootloader
	// 0x20000 = 131072
	// 0x20000 - 4 = 131068
	printf("\nListening New Kernel");
	load_kernel();
	printf("\nFinished Loading");

	asm volatile("mov %0, x20" : "=r" (dtb_addr));
	printf("\nDTB Address (Bootloader): ");
	printf_hex(dtb_addr);

	// asm volatile (
	// 	"mov x30, 0x80000;"
	// 	"ret"
	// );
	asm volatile("blr %0"
               :
               : "r" (0x80000));
}

void readcmd(char *x)
{
    char input_char;
    int input_index = 0;
    x[0] = 0;
    while( ((input_char = read_char()) != '\n'))
    {
        x[input_index] = input_char;
        ++input_index;
        printfc(input_char);
    }

    x[input_index]=0; // null char
}
