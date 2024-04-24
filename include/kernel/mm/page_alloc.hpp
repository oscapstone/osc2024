#pragma once

#include <cstdint>
#include <new.hpp>

#include "ds/list.hpp"
#include "mm/mm.hpp"
#include "util.hpp"

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
  uint64_t start_, end_;
  uint64_t length_;
  uint8_t total_order_;
  Frame* array_;
  ListHead<FreePage>* free_list_;

  void release(AllocatedPage apage);
  AllocatedPage alloc(FreePage* fpage, bool head);
  AllocatedPage split(AllocatedPage apage);
  void truncate(AllocatedPage apage, int8_t order);
  void merge(FreePage* fpage);

  void* vpn2addr(uint64_t vpn) {
    return (void*)(start_ + vpn * PAGE_SIZE);
  }
  uint64_t addr2vpn(void* addr) {
    return ((uint64_t)addr - start_) / PAGE_SIZE;
  }
  uint64_t buddy(uint64_t vpn) {
    return vpn ^ (1 << array_[vpn].order);
  }
  template <bool assume = false>
  FreePage* vpn2freepage(uint64_t vpn) {
    auto addr = vpn2addr(vpn);
    if (assume or not array_[vpn].allocated) {
      return (FreePage*)addr;
    } else {
      array_[vpn].allocated = false;
      return new (addr) FreePage;
    }
  }

 public:
  void init(uint64_t start, uint64_t end);
  void info();
  void* alloc(uint64_t size);
  void free(void* ptr);

  uint64_t start() const {
    return start_;
  }
  uint64_t end() const {
    return end_;
  }
};

extern PageAlloc page_alloc;
