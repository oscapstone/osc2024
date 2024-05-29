#pragma once

#include "ds/list.hpp"
#include "mm/mmu.hpp"
#include "string.hpp"
#include "util.hpp"

struct VMA : ListItem {
 private:
  uint64_t addr_, size_;

 public:
  const string name;
  const ProtFlags prot;

  VMA(string name, uint64_t addr, uint64_t size, ProtFlags prot)
      : ListItem{}, addr_{addr}, size_{size}, name{name}, prot{prot} {}

  VMA(const VMA& o)
      : ListItem{},
        addr_{o.addr_},
        size_{o.size_},
        name{o.name},
        prot{o.prot} {}

  uint64_t start() const {
    return addr_;
  }
  uint64_t end() const {
    return addr_ + size_;
  }
  uint64_t size() const {
    return size_;
  }
};

struct PageItem : ListItem {
  uint64_t addr;
  template <typename T>
  PageItem(T addr) : ListItem{}, addr((uint64_t)addr) {}
};

class VMM {
 public:
  PT* el0_tlb = nullptr;
  ListHead<VMA> items{};
  ListHead<PageItem> user_ro_pages{};

  VMM() = default;
  VMM(const VMM& o);
  ~VMM();

  void ensure_el0_tlb();
  int alloc_user_pages(uint64_t va, uint64_t size, ProtFlags prot);
  int map_user_phy_pages(uint64_t va, uint64_t pa, uint64_t size,
                         ProtFlags prot);

  void reset();
  void return_to_user();
};
