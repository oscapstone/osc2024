#pragma once

extern "C" {
void memzero(void* start, void* end);
void* memcpy(void* dst, const void* src, int n);
}

void memset(void* b, int c, int len);
int memcmp(const void* s1, const void* s2, int n);
int strlen(const char* s);
char* strcpy(char* dst, const char* src);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, int n);
const char* strchr(const char* s, char c);
long strtol(const char* s, const char** endptr = nullptr, int base = 0,
            int n = 0);

class string_view {
  const char* buf_;
  long size_;

 public:
  using iterator = const char*;
  string_view() : buf_(nullptr), size_(0) {}
  string_view(const char* buf) : buf_(buf), size_(strlen(buf)) {}
  string_view(const char* buf, long size) : buf_(buf), size_(size) {}
  iterator begin() {
    return buf_;
  }
  iterator end() {
    return buf_ + size_;
  }
  const char* data() {
    return buf_;
  }
  long size() const {
    return size_;
  }
  bool empty() const {
    return size_ == 0;
  }
  char operator[](long i) const {
    return buf_[i];
  }
  bool printable() const {
    for (long i = 0; i < size_; i++) {
      auto c = buf_[i];
      if (not(0x20 <= c and c <= 0x7e) and not(i + 1 == size_ and c == 0))
        return false;
    }
    return true;
  }
};

bool operator==(string_view, string_view);
