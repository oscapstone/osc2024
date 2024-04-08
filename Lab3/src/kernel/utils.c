#include "kernel/utils.h"
#include "kernel/dtb.h"

char parse(const char c){
    if(c == CR)
        return '\n';
    else    
        return c;
}

int string_len(const char* str){
    int i = 0;

    while(1){
        if(*(str + i) == '\0' || *(str + i) == '\n')
            break;
        i++;
    }

    return i;
}

// According to strcmp in string.h, return 0 if strings are the same
int string_comp(const char *str1, const char *str2){
    // this should also be 1 or -1, but consider it as future work
    if(string_len(str1) != string_len(str2))
        return 1;
    else{
        int i;
        
        for(i = 0; i < string_len(str1); i++){
            // return >0 if str1 is larger in first unmatched char
            if(str1[i] > str2[i])
                return 1;
            // return <0 if str1 is smaller in first unmatched char
            else if(str1[i] < str2[i])
                return -1;
        }

        return 0;
    }
}

int string_comp_l(const char *str1, const char *str2, int len){
    int i;
        
    for(i = 0; i < len; i++){
        // return >0 if str1 is larger in first unmatched char
        if(str1[i] > str2[i])
            return 1;
        // return <0 if str1 is smaller in first unmatched char
        else if(str1[i] < str2[i])
            return -1;
    }

    return 0;
}

void string_set(char *str, int n, int size){
    int i;
    for(i = 0; i < size; i++)
        str[i] = n;
}

void string_copy(char *dst, char *src){
    int i;

    for(i = 0; i < string_len(src); i++){
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

int h2i(const char *str, int len){
    int i;
    int res = 0;

    for(i = 0; i < len; i++){
        res = res << 4; // shift left 4 bits(equals to multiply by 16), as the first char we read is MSB

        if(str[i] >= 'A' && str[i] <= 'F'){
            res += (str[i] - 'A' + 10);
        }
        else if(str[i] >= 'a' && str[i] <= 'f'){
            res += (str[i] - 'a' + 10);
        }
        else
            res += str[i] - '0';
    }

    return res;
}

int align_offset(unsigned int i, unsigned int align){
    // (4-size%4) can get right value if size%4 != 0, so mod again to eliminate 0(if size%4 = 0, padding should be 0 instead of 4)
    return ((align - (i % align) ) % align);
}

int align_mem_offset(void* i, unsigned int align){
    // unsigned long long seemed not work properly?
    uintptr_t l = (uintptr_t)i;
    return ((align - (l % align) ) % align);
}

unsigned int BE2LE(unsigned int BE){
    unsigned int LE = 0;
    int i = 24;

    for(; i >= 0; i -= 8){
        LE |= (((BE >> i) & 0xFF) << (24 - i));
    }

    return LE;
}