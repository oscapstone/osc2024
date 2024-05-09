extern char _heap_top;//from linker script,which is the start of heap block

extern char* allocated;//the heap top now

void* simple_malloc(unsigned int size);
void* get_sp();