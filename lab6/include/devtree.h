#pragma once

#include <stdint.h>

#include "hardware.h"

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

/**
 * @brief Convert a 4-byte big-endian sequence to little-endian.
 *
 * @param s: big-endian sequence
 * @return little-endian sequence
 */
uint32_t be2le(const void *s);

void fdt_traverse(void (*callback)(void *, char *));
