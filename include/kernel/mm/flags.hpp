#pragma once

#include "ds/bitmask_enum.hpp"
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
  MAP_ANONYMOUS = 0x20,
  MAP_POPULATE = 0x008000,
  MARK_AS_BITMASK_ENUM(MAP_POPULATE),
};
