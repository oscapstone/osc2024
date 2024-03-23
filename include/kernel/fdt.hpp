#pragma once

#include <cstdint>

#include "string.hpp"

constexpr uint32_t FDT_MAGIC = 0xd00dfeed;
constexpr uint32_t LAST_COMP_VERSION = 17;

// ref: https://github.com/torvalds/linux/blob/v6.8/scripts/dtc/libfdt/libfdt.h
/*
 * libfdt - Flat Device Tree manipulation
 * Copyright (C) 2006 David Gibson, IBM Corporation.
 */

static inline const char* fdt_ld(const char p[]) {
  return p;
}
static inline uint32_t fdt_ld(const uint32_t& p) {
  const uint8_t* bp = (const uint8_t*)&p;
  return ((uint32_t)bp[0] << 24) | ((uint32_t)bp[1] << 16) |
         ((uint32_t)bp[2] << 8) | bp[3];
}
static inline uint64_t fdt_ld(const uint64_t& p) {
  const uint8_t* bp = (const uint8_t*)&p;
  return ((uint64_t)bp[0] << 56) | ((uint64_t)bp[1] << 48) |
         ((uint64_t)bp[2] << 40) | ((uint64_t)bp[3] << 32) |
         ((uint64_t)bp[4] << 24) | ((uint64_t)bp[5] << 16) |
         ((uint64_t)bp[6] << 8) | bp[7];
}

template <uint64_t sz, typename T>
static inline T align(T p) {
  return (T)(((uint64_t)p + sz - 1) & ~(sz - 1));
}

#define fdt_get_header(fdt, field) \
  (fdt_ld(((const struct fdt_header*)(fdt))->field))
#define fdt_magic(fdt)             (fdt_get_header(fdt, magic))
#define fdt_totalsize(fdt)         (fdt_get_header(fdt, totalsize))
#define fdt_off_dt_struct(fdt)     (fdt_get_header(fdt, off_dt_struct))
#define fdt_off_dt_strings(fdt)    (fdt_get_header(fdt, off_dt_strings))
#define fdt_off_mem_rsvmap(fdt)    (fdt_get_header(fdt, off_mem_rsvmap))
#define fdt_version(fdt)           (fdt_get_header(fdt, version))
#define fdt_last_comp_version(fdt) (fdt_get_header(fdt, last_comp_version))
#define fdt_boot_cpuid_phys(fdt)   (fdt_get_header(fdt, boot_cpuid_phys))
#define fdt_size_dt_strings(fdt)   (fdt_get_header(fdt, size_dt_strings))
#define fdt_size_dt_struct(fdt)    (fdt_get_header(fdt, size_dt_struct))

struct fdt_header {
  uint32_t magic;
  uint32_t totalsize;
  uint32_t off_dt_struct;
  uint32_t off_dt_strings;
  uint32_t off_mem_rsvmap;
  uint32_t version;
  uint32_t last_comp_version;
  uint32_t boot_cpuid_phys;
  uint32_t size_dt_strings;
  uint32_t size_dt_struct;
};

#define fdtrsv_get_header(fdt, field) \
  (fdt_ld(((const struct fdt_reserve_entry*)(fdt))->field))
#define fdtrsv_address(fdt) (fdtrsv_get_header(fdt, address))
#define fdtrsv_size(fdt)    (fdtrsv_get_header(fdt, size))

struct fdt_reserve_entry {
  uint64_t address;
  uint64_t size;
};

#define fdtn_get_header(fdt, field) \
  (fdt_ld(((const struct fdt_node*)(fdt))->field))
#define fdtn_name(fdt) (fdtn_get_header(fdt, name))

struct fdt_node {
  char name[];
};

#define fdtp_get_header(fdt, field) \
  (fdt_ld(((const struct fdt_prop*)(fdt))->field))
#define fdtp_len(fdt)     (fdtp_get_header(fdt, len))
#define fdtp_nameoff(fdt) (fdtp_get_header(fdt, nameoff))
#define fdtp_prop(fdt)    (fdtp_get_header(fdt, prop))

struct fdt_prop {
  uint32_t len;
  uint32_t nameoff;
  char prop[];
};

constexpr uint32_t FDT_BEGIN_NODE = 0x00000001;
constexpr uint32_t FDT_END_NODE = 0x00000002;
constexpr uint32_t FDT_PROP = 0x00000003;
constexpr uint32_t FDT_NOP = 0x00000004;
constexpr uint32_t FDT_END = 0x00000009;

class FDT {
 public:
  using fp = bool (*)(uint32_t tag, int level, const char* node_name,
                      const char* prop_name, uint32_t len,
                      const char prop_value[]);

 private:
  char* base;
  fdt_reserve_entry* reserve_entry;
  uint32_t* struct_base(int i = 0) {
    return (uint32_t*)(base + fdt_off_dt_struct(base) + i);
  }
  char* str_base(int i = 0) {
    return (char*)(base + fdt_off_dt_strings(base) + i);
  }
  void traverse_impl(int& level, uint32_t& offset, const char* node_name,
                     fp callback);

 public:
  bool init(void* addr);
  void traverse(fp callback);
};
extern FDT fdt;
