#pragma once

#include <new.hpp>

#include "ds/list.hpp"
#include "mm/mm.hpp"
#include "util.hpp"

class PageSystem {
 public:
  enum class FRAME_TYPE : uint8_t {
    FREE,
    ALLOCATED,
    NOT_HEAD,
    RESERVED,
  };
  const char* str(FRAME_TYPE type) {
    switch (type) {
      case FRAME_TYPE::FREE:
        return "free";
      case FRAME_TYPE::ALLOCATED:
        return "allocated";
      case FRAME_TYPE::NOT_HEAD:
        return "not head";
      case FRAME_TYPE::RESERVED:
        return "reserved";
    }
  }

  struct Frame {
    union {
      struct __attribute__((__packed__)) {
        FRAME_TYPE type : 2;
        uint8_t order : 6;
        uint8_t ref;
      };
      uint16_t value;
    };
    bool allocated() const {
      return type == FRAME_TYPE::ALLOCATED;
    }
    bool free() const {
      return type == FRAME_TYPE::FREE;
    }
    bool head() const {
      return type != FRAME_TYPE::NOT_HEAD;
    }
  };

  struct FreePage : ListItem<FreePage> {};
  using AllocatedPage = void*;

 private:
  bool log = true;
  uint64_t start_, end_;
  uint64_t length_;
  uint8_t total_order_;
  Frame* array_;
  ListHead<FreePage*>* free_list_;

  void release(AllocatedPage apage);
  AllocatedPage alloc(FreePage* fpage, bool head);
  AllocatedPage split(AllocatedPage apage);
  void truncate(AllocatedPage apage, uint8_t order);
  void merge(FreePage* fpage);

  void* pfn2addr(uint64_t pfn) {
    return (void*)(start_ + pfn * PAGE_SIZE);
  }
  void* pfn2end(uint64_t pfn) {
    return (void*)(start_ + (pfn + (1 << array_[pfn].order)) * PAGE_SIZE);
  }
  uint64_t addr2pfn(void* addr) {
    return ((uint64_t)addr - start_) / PAGE_SIZE;
  }
  uint64_t addr2pfn_safe(void* addr) {
    if ((uint64_t)addr < start_)
      return 0;
    if (end_ < (uint64_t)addr)
      return (end_ - start_) / PAGE_SIZE;
    return ((uint64_t)addr - start_) / PAGE_SIZE;
  }
  uint64_t buddy(uint64_t pfn) {
    return pfn ^ (1 << array_[pfn].order);
  }
  template <bool assume = false>
  FreePage* pfn2freepage(uint64_t pfn) {
    auto addr = pfn2addr(pfn);
    if (assume or array_[pfn].free()) {
      return (FreePage*)addr;
    } else {
      array_[pfn].type = FRAME_TYPE::FREE;
      return new (addr) FreePage;
    }
  }

 public:
  void preinit(uint64_t start, uint64_t end);
  void reserve(void* start, void* end);
  void init();
  void info();
  void* alloc(uint64_t size);
  void free(void* ptr);

  uint64_t start() const {
    return start_;
  }
  uint64_t end() const {
    return end_;
  }

  template <typename T>
  bool managed(T addr) {
    return start() <= (uint64_t)addr and (uint64_t) addr <= end();
  }

  template <typename T>
  uint8_t& refcnt(T addr) {
    return array_[addr2pfn((void*)addr)].ref;
  }
};

extern PageSystem mm_page;
