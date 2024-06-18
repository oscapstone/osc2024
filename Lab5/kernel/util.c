#include "util.h"

int strcmp(char* str1, char* str2){
    char* p1 = str1;
    char* p2 = str2;

    while (*p1 == *p2){
        if (*p1 == '\0')
            return 1;
        
        p1++;
        p2++;
    }

    return 0;
}

int strncmp(char* str1, char* str2, uint32_t len){
    char* p1 = str1;
    char* p2 = str2;

    for (int i = 0; i < len; i++){
        if (p1[i] != p2[i])
            return 0;

        if (p1[i] == '\0')
            return 1;
    }

    return 1;
}

void strncpy(char* src, char* dst, uint32_t len){
    for (int i = 0; i < len; i++){
        dst[i] = src[i];
        if (src[i] == '\0')
            break;
    }
}

void memcpy(char* src, char* dst, uint32_t len){
    for (int i = 0; i < len; i++){
        dst[i] = src[i];
    }
}

uint32_t strlen(char* str){
    uint32_t len = 0;

    while (*str != '\0'){
        len++;
        str++;
    }

    return len;
}

int atoi(char* val_str, int len){

    int val = 0;

    for (int i = 0; i < len && val_str[i] != '\0'; i++){
        int tmp_val = 0;
        if ('0' <= val_str[i] && val_str[i] <= '9'){
            tmp_val = val_str[i] - '0';
        }else if ('A' <= val_str[i] && val_str[i] <= 'F'){
            tmp_val = val_str[i] - 'A' + 10;
        }else if ('a' <= val_str[i] && val_str[i] <= 'f'){
            tmp_val = val_str[i] - 'a' + 10;
        }
        
        val = (val << 4) + tmp_val;
    }    
    
    return val;
}

uint32_t to_little_endian(uint32_t val){
    return  ((val >> 24) & (0x000000ff)) |
            ((val >> 8)  & (0x0000ff00)) |
            ((val << 8)  & (0x00ff0000)) |
            ((val << 24) & (0xff000000));
}

void delay(int ncycles){
    for (int i = 0; i < ncycles; i++)
        asm volatile("nop");
}

void print_dec(int dec_val){
    char val_str[20];
    int idx = 0;

    if (dec_val == 0){
        idx = 1;
        val_str[0] = '0';
    }
    
    while (dec_val > 0){
        char ch = '0' + (dec_val % 10);
        val_str[idx++] = ch;

        dec_val /= 10;
    }

    for (int i = idx-1; i >= 0; i--){
        print_char(val_str[i]);
    }
}

uint32_t read_DAIF(){
    uint32_t value;
    asm volatile ("mrs %0, DAIF" : "=r" (value));

    return value;
}

uint32_t read_core_timer_enable(){
    uint32_t value;
    asm volatile ("mrs  %0, cntp_ctl_el0;" : "=r"(value));
    return value;
}

uint32_t read_core_timer_expire(){
    uint32_t value;
    asm volatile ("mrs  %0, cntp_tval_el0" : "=r"(value));
    return value;
}