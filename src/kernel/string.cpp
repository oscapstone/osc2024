#include "string.hpp"

#include "mm/mm.hpp"
#include "nanoprintf.hpp"

char* strdup(const char* s) {
  int size = strlen(s) + 1;
  auto p = (char*)kmalloc(size);
  memcpy(p, s, size);
  return p;
}

bool operator==(string_view a, string_view b) {
  if (a.size() != b.size())
    return false;
  return !memcmp(a.data(), b.data(), a.size());
}

string operator+(const string& a, const string& b) {
  string s(a.size() + b.size() + 1);
  s += a;
  s += b;
  return s;
}

template <>
string to_hex_string(uint64_t value) {
  uint32_t size = npf_snprintf(nullptr, 0, "0x%lx", value) + 1;
  string s(size);
  s += npf_snprintf(s.data(), s.cap(), "0x%lx", value);
  return s;
}
