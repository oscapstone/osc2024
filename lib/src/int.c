#include "int.h"

int align(int num, int n) {
  int mask = ~(n - 1);
  return (num + n - 1) & mask;
}
