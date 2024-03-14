#include "string.h"

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