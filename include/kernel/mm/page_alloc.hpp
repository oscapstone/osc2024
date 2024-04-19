#pragma once

#include <_abort.hpp>
#include <cstdint>
#include <new>

#include "ds/list.hpp"
#include "util.hpp"

constexpr uint64_t PAGE_SIZE = 0x1000;

class PageAlloc {
 public:
  constexpr static int8_t FRAME_NOT_HEAD = -1;
  struct Frame {
    union {
      struct __attribute__((__packed__)) {
        bool allocated : 1;
        int order : 7;
      };
      int8_t value;
    };
    bool head() const {
      return value != FRAME_NOT_HEAD;
    }
  };

  struct FreePage : ListItem {};
  using AllocatedPage = void*;

 private:
  uint64_t start, end;
  uint64_t length;
  uint8_t total_order;
  Frame* array;
  ListHead<FreePage>* free_list;

  FreePage* release(AllocatedPage apage);
  AllocatedPage alloc(FreePage* fpage);
  AllocatedPage split(AllocatedPage apage);
  void truncate(AllocatedPage apage, int8_t order);

 public:
  void init(uint64_t start, uint64_t end);
  void* vpn2addr(uint64_t vpn) {
    return (void*)(start + vpn * PAGE_SIZE);
  }
  uint64_t addr2vpn(void* addr) {
    return ((uint64_t)addr - start) / PAGE_SIZE;
  }
  uint64_t buddy(uint64_t vpn) {
    return vpn ^ (1 << array[vpn].order);
  }
  void info();
  void* alloc(uint64_t size);
  void free(void* ptr);
};

extern PageAlloc page_alloc;
