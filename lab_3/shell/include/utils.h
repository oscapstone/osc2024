void set(long addr, unsigned int value);
void reset();
int strcmp(char* a, char* b);
void cancel_reset();
void utils_align(void *size, unsigned int s);
unsigned long utils_atoi(const char *s, int char_size);
void* simple_malloc(unsigned long size);