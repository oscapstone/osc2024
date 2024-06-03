#pragma once

#include <concepts>

#include_next "string.hpp"

char* strdup(const char* s);

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

class string {
 private:
  char *beg_, *end_, *cap_;

 public:
  string() : beg_{nullptr}, end_{nullptr}, cap_{nullptr} {}
  string(uint32_t cap) : string{} {
    reserve(cap);
  }
  string(const char* s, uint32_t n) : string{} {
    append(s, n);
  }
  string(const char* s) : string(s, strlen(s)) {}
  string(const string& o) : string{} {
    reserve(o.size());
    append(o);
  }
  ~string() {
    delete[] beg_;
  }

  operator string_view() const {
    return {data(), size()};
  }

  void reserve(uint32_t new_cap) {
    if (cap_ - beg_ < new_cap) {
      auto nstr = new char[new_cap + 1];
      uint32_t n = size();
      memcpy(nstr, beg_, n);
      beg_ = nstr;
      end_ = nstr + n;
      cap_ = nstr + new_cap;
    }
  }

  char* data() const {
    return beg_;
  }
  uint32_t size() const {
    return end_ - beg_;
  }
  uint32_t cap() const {
    return cap_ - beg_;
  }

  string& operator+=(const string& o) {
    return append(o);
  }
  string& append(const string& o) {
    return append(o.data(), o.size());
  }
  string& append(const char* s, size_t n) {
    reserve(size() + n);
    memcpy(end_, s, n);
    end_ += n;
    return *this;
  }
  string& operator+=(uint32_t size) {
    end_ += size;
    return *this;
  }
  char& operator[](uint32_t i) const {
    return data()[i];
  }
};

bool operator==(string_view, string_view);
string operator+(const string& a, const string& b);
string to_hex_string(uint64_t value);
template <typename T>
  requires(!std::is_integral_v<T>)
string to_hex_string(T value) {
  return to_hex_string((uint64_t)value);
}
