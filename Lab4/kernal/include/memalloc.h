extern char _heap_top;//from linker script,which is the start of heap block
extern char _stack_top;//from linker script,which is the start of stack block
extern char* allocated;//the heap top now

void* simple_malloc(unsigned int size);
void* get_sp();
char* mem_alin(char* ptr,int alin);