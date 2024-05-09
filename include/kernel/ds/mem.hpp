#pragma once

#include <cstdint>

#include "mm/mm.hpp"
#include "new.hpp"

struct Mem {
  char* addr;
  uint64_t size;
  int* ref;

  Mem() : addr(nullptr), size(0), ref(nullptr) {}
  Mem(uint64_t size, bool copy) {
    alloc(size, copy);
  }

  Mem(const Mem& o) : size(o.size), ref(o.ref) {
    if (o.ref) {
      (*o.ref)++;
      addr = o.addr;
    } else {
      addr = (char*)kmalloc(size);
      memcpy(addr, o.addr, size);
      fix(o, addr, size);
    }
  }

  ~Mem() {
    dealloc();
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

  bool has(void* p) const {
    return addr <= (char*)p and (char*) p < addr + size;
  }

  bool alloc(uint64_t size_, bool copy) {
    if (addr)
      return false;
    addr = (char*)kmalloc(size_, PAGE_SIZE);
    if (addr == nullptr)
      return false;
    size = size_;
    ref = copy ? nullptr : (int*)kmalloc(sizeof(int));
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
