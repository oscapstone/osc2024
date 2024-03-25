#include "reloc.hpp"

uint64_t reloc_base = (uint64_t)&reloc_impl;

uint64_t reloc_impl(uint64_t x) {
  return x - reloc_base + (uint64_t)&reloc_impl;
}
