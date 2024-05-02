#include "alloc.h"
#include "mini_uart.h"
#include "fdt.h"
#include "set.h"
#include "cpio.h"


char* now = 0x90000;

void* simple_malloc(int size) {
	now += size;
	return (char*)(now - size);
}
