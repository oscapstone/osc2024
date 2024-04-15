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
  constexpr static int8_t FRAME_ALLOCATED = -2;

  struct FreePage : ListItem {};

 private:
  uint64_t start, end;
  uint64_t length;
  uint8_t total_order;
  int8_t* array;
  ListHead<FreePage>* free_list;

  FreePage* split(FreePage* page, int8_t order);
  FreePage* newFreePage(uint64_t vpn, int8_t order) {
    array[vpn] = order;
    auto page = new (vpn2addr(vpn)) FreePage;
    free_list[order].insert_tail(page);
    return page;
  }

 public:
  void init(uint64_t start, uint64_t end);
  void* vpn2addr(uint64_t vpn) {
    return (void*)(start + vpn * PAGE_SIZE);
  }
  uint64_t addr2vpn(void* addr) {
    return ((uint64_t)addr - start) / PAGE_SIZE;
  }
  uint64_t buddy(uint64_t vpn) {
    return vpn ^ (1 << array[vpn]);
  }
  void info();
  void* alloc(uint64_t size);
  void free(void* ptr);
};

extern PageAlloc page_alloc;
