#pragma once

#include <type_traits>

#include "arm.hpp"
#include "ds/bitmask_enum.hpp"
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

inline const char* PT_levelstr(int level) {
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

enum class ProtFlags {
  NONE = 0,
  READ = 1 << 0,
  WRITE = 1 << 1,
  EXEC = 1 << 2,
  RW = READ | WRITE,
  RWX = READ | WRITE | EXEC,
  MARK_AS_BITMASK_ENUM(EXEC),
};

enum class AP : uint64_t {
  KERNEL_RW = 0b00,
  USER_RW = 0b01,
  KERNEL_RO = 0b10,
  USER_RO = 0b11,
};

enum class EntryKind : uint64_t {
  INVALID = 0,
  TABLE = 1,  // PD_TABLE
  ENTRY = 2,  // PD_BLOCK or PTE_ENTRY
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
      bool level_is_PTE : 1 = false;
      bool require_free : 1 = false;
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

  const char* kindstr() const {
    switch (kind()) {
      case EntryKind::TABLE:
        return "TABLE";
      case EntryKind::ENTRY:
        return "ENTRY";
      default:
        return "INVALID";
    }
  }

  void print(int level = -1) const;

  bool isInvalid() const {
    return (type & 1) == 0;
  }

  void set_level(int level) {
    level_is_PTE = level == PTE_LEVEL;
    if (level_is_PTE and not isInvalid())
      type = PTE_ENTRY;
  }
  bool isPTE() const {
    return level_is_PTE;
  }

  EntryKind kind() const {
    // if (isInvalid())
    //   return EntryKind::INVALID;
    if (isPTE()) {
      if (type == PTE_ENTRY)
        return EntryKind::ENTRY;
      return EntryKind::INVALID;
    } else {
      if (type == PD_TABLE)
        return EntryKind::TABLE;
      if (type == PD_BLOCK)
        return EntryKind::ENTRY;
      return EntryKind::INVALID;
    }
  }
  bool isEntry() const {
    return kind() == EntryKind::ENTRY;
  }
  bool isTable() const {
    return kind() == EntryKind::TABLE;
  }

  template <typename T = uint64_t>
  void* addr(T offset = 0) const {
    return (void*)(output_address * PAGE_SIZE + (uint64_t)offset);
  }
  void set_addr(void* addr, uint64_t new_type) {
    asm volatile("" ::: "memory");
    auto e = *this;
    e.AF = true;
    e.output_address = (uint64_t)va2pa(addr) / PAGE_SIZE;
    e.type = new_type;
    value = e.value;
    asm volatile("" ::: "memory");
  }
  template <typename T>
  void set_entry(T addr, int level, bool req_free = false) {
    set_level(level);
    require_free = req_free;
    set_addr((void*)addr, isPTE() ? PTE_ENTRY : PD_BLOCK);
  }

  PT* table() const {
    return (PT*)pa2va(addr());
  }
  void set_table(int level, PT* table) {
    set_level(level);
    set_addr((void*)table, PD_TABLE);
  }

  void alloc(int level, bool kernel = false);
  PT_Entry copy(int level) const;
};
static_assert(sizeof(PT_Entry) == sizeof(uint64_t));

struct PT {
  PT_Entry entries[TABLE_SIZE_4K];
  PT();
  PT(PT_Entry entry, int level = PGD_LEVEL);
  PT* copy();
  PT(PT* table, int level = PGD_LEVEL);
  ~PT();
  PT_Entry& walk(uint64_t start, int level, uint64_t va_start, int va_level);
  using CB = void(void* context, PT_Entry& entry, uint64_t start, int level);
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

  void set_level(int level) {
    for (uint64_t i = 0; i < TABLE_SIZE_4K; i++)
      entries[i].set_level(level);
  }

  void print(const char* name = "PageTable", uint64_t start = USER_SPACE,
             int level = PGD_LEVEL);

  template <typename T, typename R = std::conditional_t<
                            sizeof(T) == sizeof(void*), T, void*>>
  R translate(T va, uint64_t start = USER_SPACE, int level = PGD_LEVEL) {
    return (R)translate_va((uint64_t)va, start, level);
  }
  void* translate_va(uint64_t va, uint64_t start = USER_SPACE,
                     int level = PGD_LEVEL);
};

static_assert(sizeof(PT) == PAGE_SIZE);

PT* pt_copy(PT*);
void map_kernel_as_normal(char* ktext_beg, char* ktext_end);

inline void debug_TTBR(bool upper = false) {
  if (upper)
    pa2va((PT*)read_sysreg(TTBR1_EL1))->print("TTBR1_EL1", KERNEL_SPACE);
  else
    pa2va((PT*)read_sysreg(TTBR0_EL1))->print("TTBR0_EL1");
}
