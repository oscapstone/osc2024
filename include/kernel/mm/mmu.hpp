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
constexpr uint64_t MEM_START = 0;

template <typename T>
inline uint64_t lower_addr(T x) {
  return (uint64_t)x & (~ADDRESS_SPACE_TAG);
}

template <typename T,
          typename R = std::conditional_t<sizeof(T) == sizeof(void*), T, void*>>
inline R va2pa(T x) {
  return (R)(lower_addr(x));
}
template <typename T,
          typename R = std::conditional_t<sizeof(T) == sizeof(void*), T, void*>>
inline R pa2va(T x) {
  return (R)(lower_addr(x) | KERNEL_SPACE);
}

template <typename T>
uint64_t addressSpace(T x) {
  return (uint64_t)x & ADDRESS_SPACE_TAG;
}
template <typename T>
inline bool isKernelSpace(T x) {
  return addressSpace(x) == KERNEL_SPACE;
}
template <typename T>
inline bool isUserSpace(T x) {
  return addressSpace(x) == USER_SPACE;
}

struct PT;
constexpr uint64_t TABLE_SIZE_4K = 512;
constexpr uint64_t PTE_ENTRY_SIZE = PAGE_SIZE;
constexpr uint64_t PMD_ENTRY_SIZE = PTE_ENTRY_SIZE * TABLE_SIZE_4K;
constexpr uint64_t PUD_ENTRY_SIZE = PMD_ENTRY_SIZE * TABLE_SIZE_4K;
constexpr uint64_t PGD_ENTRY_SIZE = PUD_ENTRY_SIZE * TABLE_SIZE_4K;
constexpr uint64_t ENTRY_SIZE[] = {PGD_ENTRY_SIZE, PUD_ENTRY_SIZE,
                                   PMD_ENTRY_SIZE, PTE_ENTRY_SIZE};
constexpr int PGD_LEVEL = 0, PUD_LEVEL = 1, PMD_LEVEL = 2, PTE_LEVEL = 3;

enum class AP : uint64_t {
  KERNEL_RW = 0b00,
  USER_RW = 0b01,
  KERNEL_RO = 0b10,
  USER_RO = 0b11,
};

struct PT_Entry {
  union {
    struct {
      uint64_t type : 2 = PD_INVALID;
      uint64_t AttrIdx : 3 = MAIR_IDX_NORMAL_NOCACHE;
      bool NS : 1 = false;
      AP AP : 2 = AP::USER_RW;
      bool SH : 2 = false;
      bool AF : 1 = false;
      bool nG : 1 = false;
      uint64_t output_address : 36 = 0;
      uint64_t res : 4 = 0;
      bool Contiguous : 1 = false;
      bool PXN : 1 = false;
      bool UXN : 1 = false;
      uint64_t level : 2 = 0;
      uint64_t software_reserved : 2 = 0;
      uint64_t upper_atributes : 5 = 0;
    };
    uint64_t value;
  };

  const char* apstr() const {
    switch (AP) {
      case AP::KERNEL_RO:
        return "KERNEL_RO";
      case AP::KERNEL_RW:
        return "KERNEL_RW";
      case AP::USER_RO:
        return "USER_RO";
      case AP::USER_RW:
        return "USER_RW";
    }
    return "unknown_AP";
  }

  const char* typestr() const {
    if (level == PTE_LEVEL)
      return type == PD_TABLE ? "ENTRY" : "INVALID";
    switch (type) {
      case PD_TABLE:
        return "TABLE";
      case PD_BLOCK:
        return "BLOCK";
      default:
        return "INVALID";
    }
  }

  const char* levelstr() const {
    switch (level) {
      case PGD_LEVEL:
        return "PGD";
      case PUD_LEVEL:
        return "PUD";
      case PMD_LEVEL:
        return "PMD";
      case PTE_LEVEL:
        return "PTE";
    }
    return "unknown";
  }
  void print() const;

  bool isInvalid() const {
    return (type & 1) == 0;
  }
  bool isPTE() const {
    return level == PTE_LEVEL;
  }
  bool isEntry() const {
    return type == PD_BLOCK or (type == PD_TABLE and isPTE());
  }
  bool isTable() const {
    return type == PD_TABLE and not isPTE();
  }

  void* addr() const {
    return (void*)(output_address * PAGE_SIZE);
  }
  void set_addr(void* addr, uint64_t t) {
    asm volatile("" ::: "memory");
    auto e = *this;
    e.AF = true;
    e.output_address = (uint64_t)va2pa(addr) / PAGE_SIZE;
    e.type = t;
    value = e.value;
    asm volatile("" ::: "memory");
  }

  PT* table() const {
    return (PT*)pa2va(addr());
  }
  void set_table(PT* table) {
    set_addr((void*)table, PD_TABLE);
  }
};
static_assert(sizeof(PT_Entry) == sizeof(uint64_t));

struct PT {
  PT_Entry entries[TABLE_SIZE_4K];
  PT();
  PT(PT_Entry entry, int level);
  PT* copy();
  PT(PT* table);
  ~PT();
  PT_Entry& walk(uint64_t start, int level, uint64_t va_start, int va_level);
  using CB = void(void*, PT_Entry& entry, uint64_t start, int level);
  void walk(uint64_t start, int level, uint64_t va_start, uint64_t va_end,
            int va_level, CB cb_entry, void* context = nullptr);
  template <typename T, typename U>
  void walk(T va_start, U va_end, CB cb_entry, void* context = nullptr) {
    walk(USER_SPACE, PGD_LEVEL, (uint64_t)va_start, (uint64_t)va_end, PTE_LEVEL,
         cb_entry, context);
  }

  void traverse(uint64_t start, int level, CB cb_entry, CB cb_table = nullptr,
                void* context = nullptr);
  void traverse(CB cb_entry, CB cb_table = nullptr, void* context = nullptr) {
    return traverse(USER_SPACE, PGD_LEVEL, cb_entry, cb_table, context);
  }

  void set_level(uint64_t level) {
    for (uint64_t i = 0; i < TABLE_SIZE_4K; i++)
      entries[i].level = level;
  }

  void print(const char* name = "PageTable", uint64_t start = USER_SPACE,
             int level = PGD_LEVEL);
};

static_assert(sizeof(PT) == PAGE_SIZE);

void map_kernel_as_normal(char* ktext_beg, char* ktext_end);
