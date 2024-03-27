#include "header/allocator.h"
#include "header/utils.h"
#include "header/uart.h"

#define SIMPLE_MALLOC_BUFFER_SIZE 8192
static unsigned char simple_malloc_buffer[SIMPLE_MALLOC_BUFFER_SIZE];
static unsigned long simple_malloc_offset = 0;
/*extern char _heap_start;
extern char _heap_end;
static char* heap_start = &_heap_start;
static char* heap_end = &_heap_end;*/

void* simple_malloc(unsigned long size){
	//align to 8 bytes
	utils_align(&size,8);

	if(simple_malloc_offset + size > SIMPLE_MALLOC_BUFFER_SIZE) {
		//Not enough space left
		return (void*) 0;
	}
	/*if(heap_start + size > heap_end) {
		//Not enough space left
		return (void*) 0;
	}*/
	void* allocated = (void *)&simple_malloc_buffer[simple_malloc_offset];
	simple_malloc_offset += size;
	//char* allocated = (char *)heap_start + (char)size;
	//heap_start += size;
	
	uart_hex(allocated);
	uart_send_char('\n');
	return allocated;
}
