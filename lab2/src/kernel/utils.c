#include "utils.h"

unsigned int utils_transferEndian(const unsigned int* intAddr) {
    const unsigned char* bytes = (const unsigned char*) intAddr;
    unsigned int result = (unsigned int)bytes[0] << 24 | (unsigned int) bytes[1] << 16 |(unsigned int)bytes[2] << 8 |(unsigned int)bytes[3];
    return result;
}

int utils_strncmp(const char* str1, const char* str2, unsigned int len) {
    for(unsigned int i = 0; i < len; i++) {
        if (str1[i] != str2[i])
            return 0;
    }
    return 1;
}

int string_compare(const char* str1,const char* str2) {
	for(;*str1 !='\n'||*str2 !='\0';str1++,str2++){
		if(*str1 != *str2) return 0;
        if (*str1 == '\0' || *str2 == '\0' || *str1 == '\n' || *str2 == '\n')
            return 1;
	}
	return 1;
}

unsigned long atoi(const char *s, int char_size) {
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

void align(void *size, unsigned int s) {
	unsigned long* x = (unsigned long*) size;
	unsigned long mask = s-1;
	*x = ((*x) + mask) & (~mask);
}

U32 utils_align_up(U32 size, int alignment) {
  return (size + alignment - 1) & ~(alignment-1);
}

U64 utils_strlen(const char *s) {
    U64 i = 0;
	while (s[i]) i++;
	return i + 1;
}



