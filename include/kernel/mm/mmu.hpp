#pragma once

#include <type_traits>

#include "arm.hpp"
#include "mm/mm.hpp"
#include "util.hpp"

extern char __upper_PGD[];
extern char __upper_PUD[];
extern char __upper_end[];

constexpr uint64_t ADDRESS_SPACE_TAG = 0xFFFF000000000000;
constexpr uint64_t KERNEL_SPACE = 0xFFFF000000000000;
constexpr uint64_t USER_SPACE = 0;

template <typename T,
          typename R = std::conditional_t<sizeof(T) == sizeof(void*), T, void*>>
inline R va2pa(T x) {
  return (R)((uint64_t)x - KERNEL_SPACE);
}
template <typename T,
          typename R = std::conditional_t<sizeof(T) == sizeof(void*), T, void*>>
inline R pa2va(T x) {
  return (R)((uint64_t)x + KERNEL_SPACE);
}

template <typename T>
inline bool isKernelSpace(T x) {
  return ((uint64_t)x & ADDRESS_SPACE_TAG) == KERNEL_SPACE;
}
template <typename T>
inline bool isUserSpace(T x) {
  return ((uint64_t)x & ADDRESS_SPACE_TAG) == USER_SPACE;
}

struct PageTable;
constexpr uint64_t TABLE_SIZE_4K = 512;
constexpr uint64_t PTE_ENTRY_SIZE = PAGE_SIZE;
constexpr uint64_t PMD_ENTRY_SIZE = PTE_ENTRY_SIZE * TABLE_SIZE_4K;
constexpr uint64_t PUD_ENTRY_SIZE = PMD_ENTRY_SIZE * TABLE_SIZE_4K;
constexpr uint64_t PGD_ENTRY_SIZE = PUD_ENTRY_SIZE * TABLE_SIZE_4K;

struct PageTableEntry {
  uint64_t upper_atributes : 10 = 0;
  bool UXN : 1 = false;
  bool PXN : 1 = false;
  bool Contiguous : 1 = false;
  uint64_t output_address : 36 = 0;
  bool nG : 1 = false;
  bool AF : 1 = false;
  bool SH : 1 = false;
  bool RDONLY : 1 = false;
  bool KERNEL : 1 = false;
  bool NS : 1 = false;
  uint64_t AttrIdx : 3 = MAIR_DEVICE_nGnRnE;
  uint64_t type : 2 = PD_INVALID;

  bool isInvalid() {
    return (type & 1) == 0;
  }
  bool isBlock() {
    return type == PD_BLOCK;
  }

  void* addr() {
    return (void*)(output_address * PAGE_SIZE);
  }
  void set_addr(void* addr) {
    output_address = (uint64_t)addr / PAGE_SIZE;
  }

  PageTable* table() {
    return (PageTable*)addr();
  }
  void set_table(PageTable* table) {
    set_addr((void*)table);
  }
};
static_assert(sizeof(PageTableEntry) == sizeof(uint64_t));

struct PageTable {
  PageTableEntry entries[TABLE_SIZE_4K];
  PageTable();
  PageTable(PageTableEntry entry, uint64_t entry_size);
  PageTableEntry& walk(char* table_start, uint64_t entry_size, char* addr,
                       uint64_t size);
  using CB = void(PageTableEntry&);
  void walk(char* table_start, uint64_t entry_size, char* start, char* end,
            CB callback);
  void walk(CB callback);
};

static_assert(sizeof(PageTable) == PAGE_SIZE);

void map_kernel_as_normal(void* kernel_start, void* kernel_end);
