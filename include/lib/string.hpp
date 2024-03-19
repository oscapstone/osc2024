#pragma once

#include "pair.hpp"

extern "C" {
void memzero(void* start, void* end);
void* memcpy(void* dst, const void* src, int n);
}

int memcmp(const void* s1, const void* s2, int n);
int strlen(const char* s);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, int n);
long strtol(const char* s, const char** endptr = nullptr, int base = 0,
            int n = 0);

class string_view {
  char* const buf_;
  int size_;

 public:
  using iterator = char*;
  // TODO: const char* -> chat*
  string_view(const char* buf, int size = -1)
      : buf_((char*)buf), size_(size == -1 ? strlen(buf) : size) {}
  iterator begin() {
    return buf_;
  }
  iterator end() {
    return buf_ + size_;
  }
  char* data() {
    return buf_;
  }
  int size() const {
    return size_;
  }
};

bool operator==(string_view, string_view);
