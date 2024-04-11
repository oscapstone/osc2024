#include "header/utils.h"
#include "header/uart.h"

int ut_st_comp(char* str1,char* str2) {
  //uart_send_string("compare\n");
  for(;*str1 !='\n'||*str2 !='\0';str1++,str2++){
    if(*str1 != *str2) 
    {
      // uart_send_char((unsigned int) *str1);
      // uart_send_string(str2);
      // uart_send_string("\n");
      return 0;
    }
    else if(*str1 == '\n' && *str2 =='\0') return 1;
  }
  return 1;
}

int ut_string_compare(char* str1,char* str2) {
  //uart_send_string("compare\n");
  for(;*str1 !='\0'||*str2 !='\0';str1++,str2++){
    if(*str1 != *str2) 
    {
      //uart_send_char((unsigned int) *str1);
      //uart_send_string(str2);
      //uart_send_string("\n");
      return 0;
    }
    else if(*str1 == '\0' && *str2 =='\0') return 1;
  }
  return 1;
}

unsigned long ut_atoi(const char *s, int char_size) {
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

void ut_align(void *size, unsigned int s) {
	unsigned long* x = (unsigned long*) size;
	unsigned long mask = s-1;
	*x = ((*x) + mask) & (~mask);
}

uint32_t ut_align_up(uint32_t size, int alignment) {
  return (size + alignment - 1) & ~(alignment-1);
}

uint32_t ut_strlen(const char *s) {
    uint32_t i = 0;
	while (s[i]) i++;
	return i+1;
}