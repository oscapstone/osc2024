#include "stdlib.h"
#include "limits.h"

int atoi(const char *s) {
  long long result = 0; // prevent overflow
  int sign = 1;
  int i = 0;

  while (s[i] == ' ')
    i++;

  if (s[i] == '-') {
    sign = -1;
    i++;
  } else if (s[i] == '+')
    i++;

  while (s[i] >= '0' && s[i] <= '9') {
    result = result * 10 + (s[i] - '0');
    i++;
  }

  return sign * (int)result;
}
