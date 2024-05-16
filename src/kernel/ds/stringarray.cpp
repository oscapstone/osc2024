#include "ds/stringarray.hpp"

#include "string.hpp"

void StringArray::init(const char* const from[]) {
  if (not from)
    return;
  if (buf) {
    delete[] buf;
    buf = nullptr;
    buf_size = 0;
  }
  if (array) {
    delete[] array;
    array = nullptr;
    size = 0;
  }
  for (auto it = from; *it; it++) {
    buf_size += strlen(*it) + 1;
    size += 1;
  }
  buf = new char[buf_size];
  array = new char*[size + 1];
  auto jt = array;
  auto kt = buf;
  for (auto it = from; *it; it++) {
    *(jt++) = kt;
    kt = strcpy(kt, *it) + 1;
  }
  *jt = nullptr;
}

StringArray::~StringArray() {
  if (buf)
    delete[] buf;
  if (array)
    delete[] array;
}
