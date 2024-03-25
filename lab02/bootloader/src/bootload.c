#include "bootload.h"

// uint64_t* BootHead = (uint64_t*)KernelAddr;
// uint64_t* CopyHead = (uint64_t*)BootAddr;
// uint64_t* bss_end  = (uint64_t*)(&__bss_end);

void reallocate()
{
	uint64_t* BootHead = (uint64_t*)KernelAddr;
	uint64_t* CopyHead = (uint64_t*)BootAddr;
	uint64_t* bss_end  = (uint64_t*)(&__bss_end);
	int i = 0;
	while(BootHead != bss_end){
		printf("\n");
		printf_hex(i);
		i++;
		*CopyHead = *BootHead;
		CopyHead++;
		BootHead++;
	}
	printf("\nCopy done.");
}

void load_kernel()
{
	unsigned int kernel_size = 0;
	for(int i=0; i<4; i++){
		kernel_size <<= 8;
		kernel_size |= (uart_recv() & 0xff);
	}

	printf("\nKernel Size: ");
	printf_hex(kernel_size);
	printf("\n");

	unsigned char* kernel = (unsigned char*)(0x80000);
	unsigned char* kernel_ptr = kernel;
	for(unsigned int i=0; i<kernel_size; i++){
		*kernel_ptr++ = uart_recv();
	}
}
