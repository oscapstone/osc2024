#pragma once

#include <cstdint>

#include "ds/list.hpp"
#include "util.hpp"

constexpr uint64_t PAGE_SIZE = 0x1000;

class PageAlloc {
 public:
  constexpr static int8_t FRAME_NOT_HEAD = -1;
  constexpr static int8_t FRAME_ALLOCATED = -2;

  struct Page : ListItem {};

 private:
  uint64_t start, end;
  uint64_t size;
  uint8_t order;
  int8_t* array;
  ListHead<Page>* free_list;

 public:
  void init(uint64_t start, uint64_t end);
  void* vpn2addr(uint64_t vpn) {
    return (void*)(start + vpn * PAGE_SIZE);
  }
  void info();
};

extern PageAlloc page_alloc;
