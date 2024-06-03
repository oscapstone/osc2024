#pragma once

#include "ds/list.hpp"
#include "mm/flags.hpp"
#include "mm/mmu.hpp"
#include "string.hpp"
#include "util.hpp"

struct VMA : ListItem<VMA> {
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

struct PageItem : ListItem<PageItem> {
  uint64_t addr;
  template <typename T>
  PageItem(T addr) : ListItem{}, addr((uint64_t)addr) {}
  PageItem(const PageItem& o) : ListItem{}, addr(o.addr) {}
};

class VMM {
 public:
  PT* ttbr0 = (PT*)INVALID_ADDRESS;
  ListHead<VMA*> vmas{};
  ListHead<PageItem*> user_ro_pages{};

  VMM() = default;
  VMM(const VMM& o);
  ~VMM();

  bool vma_overlap(uint64_t va, uint64_t size);
  uint64_t vma_addr(uint64_t va, uint64_t size);
  void vma_add(string name, uint64_t addr, uint64_t size, ProtFlags prot);
  VMA* vma_find(uint64_t va);
  void vma_print();

  void ensure_ttbr0();
  [[nodiscard]] uint64_t mmap(uint64_t va, uint64_t size, ProtFlags prot,
                              MmapFlags flags, const char* name);
  int munmap(uint64_t va, uint64_t size);
  [[nodiscard]] uint64_t map_user_phy_pages(uint64_t va, uint64_t pa,
                                            uint64_t size, ProtFlags prot,
                                            const char* name);

  void reset();
  void return_to_user();
};

VMM* current_vmm();
template <typename T,
          typename R = std::conditional_t<sizeof(T) == sizeof(void*), T, void*>>
R translate_va_to_pa(T va, uint64_t start = USER_SPACE, int level = PGD_LEVEL) {
  return (R)current_vmm()->ttbr0->translate_va((uint64_t)va, start, level);
}

template <typename T>
[[nodiscard]] uint64_t mmap(T va, uint64_t size, ProtFlags prot,
                            MmapFlags flags, const char* name) {
  return current_vmm()->mmap((uint64_t)va, size, prot, flags, name);
}

template <typename T>
int munmap(T va, uint64_t size) {
  return current_vmm()->munmap((uint64_t)va, size);
}

template <typename T, typename U>
[[nodiscard]] uint64_t map_user_phy_pages(T va, U pa, uint64_t size,
                                          ProtFlags prot, const char* name) {
  return current_vmm()->map_user_phy_pages((uint64_t)va, (uint64_t)pa, size,
                                           prot, name);
}

template <typename T>
[[nodiscard]] VMA* find_vma(T va) {
  return current_vmm()->vma_find((uint64_t)va);
}
