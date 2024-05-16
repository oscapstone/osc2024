#pragma once

#include <cstdint>

struct StringArray {
  uint32_t buf_size, size;
  char* buf;
  char** array;

  char* operator[](uint32_t idx) {
    return idx < size ? array[idx] : nullptr;
  };

  StringArray() : buf_size(0), size(0), buf(nullptr), array(nullptr) {}
  StringArray(const char* const from[]) : StringArray{} {
    init(from);
  }
  void init(const char* const from[]);
  // TODO: copy constructor
  StringArray(const StringArray&) = delete;
  ~StringArray();
};
