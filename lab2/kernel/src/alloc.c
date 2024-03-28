#include "alloc.h"
#include "mini_uart.h"

// memory at [0x90000, 0x20000000)

extern char _memory[];

char* now = _memory;

void* simple_malloc(int size) {
	now += size;
	if (now >= 0x20000000) {
		uart_printf("out of memory\n");
		now -= size;
		return 0;
	}
	return (char*)(now - size);
}
