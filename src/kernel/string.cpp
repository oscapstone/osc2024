#include "string.hpp"

#include "mm/mm.hpp"
char* strdup(const char* s) {
  int size = strlen(s) + 1;
  auto p = (char*)kmalloc(size);
  memcpy(p, s, size);
  return p;
}
