#pragma once

#include "ds/bitmask_enum.hpp"
#include "ds/list.hpp"
#include "mm/mmu.hpp"
#include "string.hpp"
#include "util.hpp"

enum class ProtFlags : uint64_t {
  NONE = 0,
  READ = 1 << 0,
  WRITE = 1 << 1,
  EXEC = 1 << 2,
  RX = READ | EXEC,
  RW = READ | WRITE,
  RWX = READ | WRITE | EXEC,
  MARK_AS_BITMASK_ENUM(EXEC),
};

enum class MmapFlags : uint64_t {
  NONE = 0,
  MAP_ANONYMOUS = 0x0800,
  MAP_POPULATE = 0x10000,
  MARK_AS_BITMASK_ENUM(MAP_POPULATE),
};

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

  template <typename T>
  bool contain(T addr) const {
    return start() <= (uint64_t)addr and (uint64_t) addr <= end();
  }

  bool overlap(uint64_t addr, uint64_t size) const {
    return addr <= end() and start() <= addr + size;
  }
};

struct PageItem : ListItem {
  uint64_t addr;
  template <typename T>
  PageItem(T addr) : ListItem{}, addr((uint64_t)addr) {}
  PageItem(const PageItem& o) : ListItem{}, addr(o.addr) {}
};

class VMM {
 public:
  PT* el0_pgd = nullptr;
  ListHead<VMA> vmas{};
  ListHead<PageItem> user_ro_pages{};

  VMM() = default;
  VMM(const VMM& o);
  ~VMM();

  bool vma_overlap(uint64_t va, uint64_t size);
  uint64_t vma_addr(uint64_t va, uint64_t size);
  void vma_add(string name, uint64_t addr, uint64_t size, ProtFlags prot);
  VMA* vma_find(uint64_t va);
  void vma_print();

  void ensure_el0_pgd();
  [[nodiscard]] uint64_t mmap(uint64_t va, uint64_t size, ProtFlags prot,
                              MmapFlags flags, const char* name);
  [[nodiscard]] uint64_t map_user_phy_pages(uint64_t va, uint64_t pa,
                                            uint64_t size, ProtFlags prot,
                                            const char* name);

  void reset();
  void return_to_user();
};
