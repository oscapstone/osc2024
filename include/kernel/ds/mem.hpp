#pragma once

#include <cstdint>

#include "mm/mm.hpp"
#include "new.hpp"

struct Mem {
  char* addr;
  uint64_t size;
  int* ref;

  Mem() : addr(nullptr), size(0), ref(nullptr) {}
  Mem(uint64_t size, bool copy) : Mem{} {
    alloc(size, copy);
  }

  Mem(const Mem& o) : size(o.size), ref(o.ref) {
    if (o.addr) {
      if (ref) {
        (*ref)++;
        addr = o.addr;
      } else {
        addr = (char*)kmalloc(size, PAGE_SIZE);
        memcpy(addr, o.addr, size);
        fix(o, addr, size);
      }
    }
  }

  ~Mem() {
    dealloc();
  }

  void* end(uint64_t off = 0) {
    return addr + size - off;
  }

  void fix(const Mem& o, void* faddr, uint64_t fsize) {
    if (ref and ref == o.ref)
      return;
    for (uint64_t off = 0; off < fsize; off += sizeof(char*)) {
      auto it = (char**)((char*)faddr + off);
      auto value = *it;
      if (o.has(value))
        *it = addr + (value - o.addr);
    }
  }
  void* fix(const Mem& o, void* value) {
    if (o.has(value))
      return addr + ((char*)value - o.addr);
    return value;
  }

  bool has(void* p) const {
    return addr <= (char*)p and (char*) p < addr + size;
  }

  bool alloc(uint64_t size_, bool copy) {
    if (addr)
      return false;
    size_ = align<PAGE_SIZE>(size_);
    addr = (char*)kmalloc(size_, PAGE_SIZE);
    if (addr == nullptr)
      return false;
    size = size_;
    if (copy) {
      ref = nullptr;
    } else {
      ref = (int*)kmalloc(sizeof(int));
      *ref = 1;
    }
    return true;
  }

  void dealloc() {
    if (addr and (not ref or --(*ref) == 0)) {
      kfree(addr);
      kfree(ref);
    }
    new (this) Mem;
  }
  operator bool() const {
    return addr != nullptr;
  }
};
