#include "utils.h"
#include "uart.h"
#include <stdint.h>
#include <stddef.h>

int utils_string_compare(const char* str1,const char* str2) {

    for(;*str1 !='\n'||*str2 !='\0';str1++,str2++){

        if(*str1 != *str2){
        return 0;
        }  
        else if(*str1 == '\n' || *str2 =='\0'){
        return 1;
        }

    }
    return 1;

}

char* utils_api_analysis(char* str1){

    while(*str1){

        if(*str1 == '('){
            break;
        }
        str1++;
    }
    return str1;
}

int utils_api_compare(const char* str1,const char* str2) {

    for(;*str1 !='('||*str2 !='(';str1++,str2++){

        if(*str1 != *str2){
        return 0;
        }  
    }
    return 1;

}

int utils_api_elem_count(const char* str1){
    int count = 0;
    if(*(str1+1) == ')'){
        return 0;
    }
    while(*str1){
        if(*str1 == ',' || *str1 == ')'){
            count++;
        }
        str1++;
    }
    return count;
}

int utils_find_api(const char* str1) {
    int bracket_before = 0;
    int bracket_after = 0;
    while(*str1 !='\n'){

        if(*str1 == '('){
        bracket_before = 1;
        }
        else if(bracket_before && *str1 == ')'){
        bracket_after = 1;
        }

        if(bracket_after && bracket_after){
        return 1;
        }

        str1++;
    }
    return 0;
}

void utils_api_get_elem(const char* str1, char** buff1, char** buff2){
    char* copy = (char*) str1;
    char* commaPos = copy;
    char* bracketPos = copy;
    while (*commaPos != ',') {
        ++commaPos;
    }
    while (*bracketPos != ')') {
        ++bracketPos;
    }

    if(*commaPos ==','){
        *commaPos = '\0';
        *bracketPos = '\0';
        *buff1 = copy+1;
        *buff2 = commaPos+1;
    }
}

unsigned long utils_hex2dec(const char *s, int char_size) {
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

void utils_align(void *size, unsigned int s) {
	unsigned long* x = (unsigned long*) size;
	unsigned long mask = s-1;
	*x = ((*x) + mask) & (~mask);
}

uint32_t utils_align_up(uint32_t size, int alignment) {
    return (size + alignment - 1) & ~(alignment-1);
}

size_t utils_strlen(const char *s) {
    size_t i = 0;
    while (s[i]) i++;
    return i+1;
}

int utils_keyword_compare(const char* str1,const char* str2) {

    while(*str1 !='\0' && *str2 !='\0'){

        if(*str1 != *str2){
        return 0;
        }
        else{
        str1++;
        str2++;
        }  
    }
    return 1;
}

int utils_str2int(const char* s){
    int num = 0;
    while(*s){
        if(*s == '\0' || *s == '\n') break;
        num = num *10;
        num = num + (*s - '0');
        s++;
    }
    return num;
}