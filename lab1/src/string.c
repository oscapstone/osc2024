
#include "string.h"

int strncmp(const char *str1, const char *str2, size_t size) {
  int index = 0;

  while (index < size) {
    if (str1[index] != str2[index]) {
      return -1;
    }

    // str2[index] == '\0' too
    if (str1[index] == '\0') {
      return 0;
    }

    index++;
  }

  return 0;
}
