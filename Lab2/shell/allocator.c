#include "header/allocator.h"
#include "header/utils.h"
#include "header/uart.h"

extern char _end;
static char* header = &_end;

void* simple_malloc(unsigned long size){
	//align to 8 bytes
	//utils_align(&size,8);

	void* allocated = (void *)header;
	header += size;
	
	return allocated;
}
