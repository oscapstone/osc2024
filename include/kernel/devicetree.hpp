#pragma once

#include <cstdint>

#define BIG_ENDIAN_FIELD(type, name) \
  type _##name; \
  type name() const { \
    return __builtin_bswap32(_##name); \
  }

struct fdt_header {
  BIG_ENDIAN_FIELD(uint32_t, magic);
  BIG_ENDIAN_FIELD(uint32_t, totalsize);
  BIG_ENDIAN_FIELD(uint32_t, off_dt_struct);
  BIG_ENDIAN_FIELD(uint32_t, off_dt_strings);
  BIG_ENDIAN_FIELD(uint32_t, off_mem_rsvmap);
  BIG_ENDIAN_FIELD(uint32_t, version);
  BIG_ENDIAN_FIELD(uint32_t, last_comp_version);
  BIG_ENDIAN_FIELD(uint32_t, boot_cpuid_phys);
  BIG_ENDIAN_FIELD(uint32_t, size_dt_strings);
  BIG_ENDIAN_FIELD(uint32_t, size_dt_struct);

  static constexpr uint32_t MAGIC = 0xd00dfeed;
  bool valid() const {
    return magic() == MAGIC;
  }
};

struct fdt_reserve_entry {
  BIG_ENDIAN_FIELD(uint64_t, address);
  BIG_ENDIAN_FIELD(uint64_t, size);
};

constexpr uint32_t FDT_BEGIN_NODE = 0x00000001;
constexpr uint32_t FDT_END_NODE = 0x00000002;
constexpr uint32_t FDT_PROP = 0x00000003;
constexpr uint32_t FDT_NOP = 0x00000004;
constexpr uint32_t FDT_END = 0x00000009;

class DeviceTree {
  static constexpr int LAST_COMP_VERSION = 17;

  char* base;
  fdt_header* header;
  fdt_reserve_entry* reserve_entry;

 public:
  bool init(void* addr);
};
extern DeviceTree dt;
