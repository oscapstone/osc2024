#pragma once

#include "util.hpp"

#define PARTITION_ID_FAT32_CHS 0x0B

struct CHS {
  unsigned H : 8;
  unsigned S : 6;
  unsigned C : 10;
} __attribute__((__packed__));

static_assert(sizeof(CHS) == 3);

struct PartitionEntry {
  uint8_t status : 7, active : 1;
  CHS first_sector_CHS;
  uint8_t type;
  CHS last_sector_CHS;
  uint32_t first_sector_LBA;
  uint32_t num_sectors;
} __attribute__((__packed__));

static_assert(sizeof(PartitionEntry) == 16);

struct MBR {
  union {
    struct {
      char code[446];
      PartitionEntry entry[4];
      unsigned char sig[2];
    };
    char buf[512];
  };

  bool valid() const {
    return sig[0] == 0x55 and sig[1] == 0xAA;
  }
} __attribute__((__packed__));

static_assert(sizeof(MBR) == 512);
