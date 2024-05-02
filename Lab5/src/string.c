#include "string.h"
#include "mini_uart.h"
#include "heap.h"
#include <stddef.h>

int strlen(const char* str){
  int i = 0;
  while(*str++){
    i++;
  }
  return i;
}

int strcmp(const char* str1, const char* str2){
  while (1)
  {
    if(*str1 == '\0' || *str2 == '\0')
      break;
    else if(*str1 != *str2)
      break;
    else if(*str1++ == *str2++)
      continue;
  }
  return (*str1 == *str2) ? 0 : 1;
}

int strcpy(char* str1, const char* str2){
  while(*str2 != '\0'){
    if(str1 == NULL)
      return -1;
    else *str1++ = *str2++;
  }
  *str1 = '\0';
  return 0;
}

char *strdup(const char* str){
  int len = strlen(str);
  // uart_sends("len: ");
  // uart_sendh(len);
  // uart_sends("\n");
  char* s = (char*)simple_malloc(len+1);
  if(s)
    strcpy(s,str);

  return s;
}

char *strtok(char* str, const char* delimeter){
  static char *last = NULL;
  char *start = NULL;

  if(str != NULL){
    last = str;
  }

  while(*last != '\0'){

    int isdelim = 0;

    for(const char *d = delimeter; *d != '\0'; d++){
      if(*last == *d){
        isdelim = 1;
        break;
      }
    }

    if(!isdelim && !start){
      start = last;
    }
    else if(isdelim && start){
      *last++ = '\0';
      return start;
    }
    last++;
  }

  return start;
}

int memcmp(const void *s1, const void *s2, int n){
  const unsigned char *a = s1, *b = s2;
  for(int i = 0; i < n; i++){
    if(a[i] < b[i])
      return -1;
    else if(a[i] > b[i])
      return 1;
  }
  return 0;
}

void memset(void* ptr, int value, int n) {
    unsigned char* p = (unsigned char*)ptr;
    for (size_t i = 0; i < n; ++i) {
        p[i] = (unsigned char)value;
    }
    return;
}

int hstr2int(const char* str, int len){
  int n = 0;
  while(len--){
    if(*str >= '0' && *str <= '9')
      n = (n * 16) + (*str++ - '0');
    else if(*str >= 'a' && *str <= 'f')
      n = (n * 16) + (*str++ - 'a' + 10);
    else if(*str >= 'A' && *str <= 'F')
      n = (n * 16) + (*str++ - 'A' + 10);
  }
  return n;
}

int gets(char *str){
  char *tmp = str;
  for (int i = 0; i < 256; i++)
  {
    *tmp = uart_recv();
    uart_sendc(*tmp);
    if (*tmp == 127){
      i--;
      if(i >= 0){
        i--;
        *tmp-- = 0;
        uart_sendc('\b');
        uart_sendc(' ');
        uart_sendc('\b');
      }
    }
    else if (*tmp == '\n'){
      break;
    }
    else tmp++;
  }
  *tmp = '\0';
  return 0;
}

int atoi(char *str){
  int n = 0;
  while(*str){
    n = n*10 + ((*str)-'0');
    str++;
  }
  return n;
}
