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

char *itoa(int value, char *s) {
  int idx = 0;

  if (value < 0) {
    value *= -1;
    s[idx++] = '-';
  }

  char tmp[10];
  int tidx = 0;

  // read from least significant difit
  do {
    tmp[tidx++] = '0' + value % 10;
    value /= 10;
  } while (value != 0 && tidx < 11);

  // revserse
  for (int i = tidx - 1; i >= 0; i--) {
    s[idx++] = tmp[i];
  }

  s[idx] = '\0';

  return s;
}
