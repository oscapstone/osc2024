#include "string.h"
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

int strcpy(char* str1, const char* str2){
  while(*str2 != '\0'){
    if(str1 == NULL)
      return -1;
    else *str1++ = *str2++;
  }
  *str1 = '\0';
  return 0;
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