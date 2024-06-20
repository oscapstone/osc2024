#define PM_PASSWORD 0x5a000000
#define PM_RSTC 0x3F10001c
#define PM_WDOG 0x3F100024
#define SIMPLE_MALLOC_BUFFER_SIZE 8192

unsigned long utils_atoi(const char *s, int char_size) {
    unsigned long num = 0;
    for (int i = 0; i < char_size; i++) {
        num = num * 16;
        if (*s >= '0' && *s <= '9') {
            num += (*s - '0');
        } else if (*s >= 'A' && *s <= 'F') {
            num += (*s - 'A' + 10);
        } else if (*s >= 'a' && *s <= 'f') {
            num += (*s - 'a' + 10);
        }
        s++;
    }
    return num;
}

int _pow(int base, int exp){
    int temp = base;
    for(int i=0;i<exp;i++){
        base *= temp;
    }
    return base;
}


/**
 * compare two string, given s1 and s2
 * return 0 if the both string are equal;
 * return -1 if s1 is less than s2;
 * return 1 if s1 is greater than s2
 */
int strcmp(char* a, char* b){
    int value = 0;
    while(*a != '\0' && *b != '\0'){
        if(*a < *b){
            return -1;
        }
        else if(*a > *b){
            return 1;
        }
        else{
            a++;
            b++;
        }
    }
    if(*a == '\0'){
        value++;
    }
    if(*b == '\0'){
        value--;
    }
    return value;
}

/**
 * set up given value to given address
 */
void set(long addr, unsigned int value) {
    volatile unsigned int* point = (unsigned int*)addr;
    *point = value;
}


void utils_align(void *size, unsigned int s) {
	unsigned long* x = (unsigned long*) size;
	unsigned long mask = s-1;
	*x = ((*x) + mask) & (~mask);
}

/**
 * reboot respberry pi
 * if tick=0, reboot instantly;
 * otherwise, wait for tick seconds to reboot
 */
void reset(int tick) {                 // reboot after watchdog timer expire
    set(PM_RSTC, PM_PASSWORD | 0x20);  // full reset
    if(tick != 0){
        set(PM_WDOG, PM_PASSWORD | tick);  // number of watchdog tick
    }
}

/**
 * clear reboot register
 */
void cancel_reset() {
    set(PM_RSTC, PM_PASSWORD | 0);  // full reset
    set(PM_WDOG, PM_PASSWORD | 0);  // number of watchdog tick
}


static unsigned char simple_malloc_buffer[SIMPLE_MALLOC_BUFFER_SIZE];
static unsigned long simple_malloc_offset = 0;

void* simple_malloc(unsigned long size){
	//align to 8 bytes
	utils_align(&size,8);

	if(simple_malloc_offset + size > SIMPLE_MALLOC_BUFFER_SIZE) {
		//Not enough space left
		return (void*) 0;
	}
	void* allocated = (void *)&simple_malloc_buffer[simple_malloc_offset];
	simple_malloc_offset += size;
	
	return allocated;
}