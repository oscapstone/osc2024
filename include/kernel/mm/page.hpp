#pragma once

#include <cstdint>
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
        int order : 6;
      };
      int8_t value;
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

  struct FreePage : ListItem {};
  using AllocatedPage = void*;

 private:
  bool log = true;
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
  void* vpn2end(uint64_t vpn) {
    return (void*)(start_ + (vpn + (1 << array_[vpn].order)) * PAGE_SIZE);
  }
  uint64_t addr2vpn(void* addr) {
    return ((uint64_t)addr - start_) / PAGE_SIZE;
  }
  uint64_t addr2vpn_safe(void* addr) {
    if ((uint64_t)addr < start_)
      return 0;
    if (end_ < (uint64_t)addr)
      return (end_ - start_) / PAGE_SIZE;
    return ((uint64_t)addr - start_) / PAGE_SIZE;
  }
  uint64_t buddy(uint64_t vpn) {
    return vpn ^ (1 << array_[vpn].order);
  }
  template <bool assume = false>
  FreePage* vpn2freepage(uint64_t vpn) {
    auto addr = vpn2addr(vpn);
    if (assume or array_[vpn].free()) {
      return (FreePage*)addr;
    } else {
      array_[vpn].type = FRAME_TYPE::FREE;
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
};

extern PageSystem mm_page;
