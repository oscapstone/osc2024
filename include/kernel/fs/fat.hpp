#pragma once

#include "ds/bitmask_enum.hpp"
#include "util.hpp"

struct FAT_BPB {
  uint8_t BS_jmpBoot[3];
  char BS_OEMName[8];
  uint16_t BPB_BytsPerSec;
  uint8_t BPB_SecPerClus;
  uint16_t BPB_RsvdSecCnt;
  uint8_t BPB_NumFATs;
  uint16_t BPB_RootEntCnt;
  uint16_t BPB_TotSec16;
  uint8_t BPB_Media;
  uint16_t BPB_FATSz16;
  uint16_t BPB_SecPerTrk;
  uint16_t BPB_NumHeads;
  uint32_t BPB_HiddSec;
  uint32_t BPB_TotSec32;

  uint32_t BPB_FATSz32;
  uint16_t BPB_ExtFlags;
  uint16_t BPB_FSVer;
  uint32_t BPB_RootClus;
  uint16_t BPB_FSInfo;
  uint16_t BPB_BkBootSec;
  char BPB_Reserved[12];
  uint8_t BS_DrvNum;
  char BS_Reserved1[1];
  uint8_t BS_BootSig;
  uint32_t BS_VolID;
  char BS_VolLab[11];
  char BS_FilSysType[8];
} __attribute__((__packed__));

struct FAT32_FSInfo {
  uint32_t FSI_LeadSig;
  char FSI_Reserved1[480];
  uint32_t FSI_StrucSig;
  uint32_t FSI_Free_Count;
  uint32_t FSI_Nxt_Free;
  char FSI_Reserved2[12];
  uint32_t FSI_TrailSig;

  bool valid() const {
    return FSI_LeadSig == 0x41615252 and FSI_StrucSig == 0x61417272 and
           FSI_TrailSig == 0xAA550000;
  }
} __attribute__((__packed__));

constexpr uint32_t FAT_FREE = 0;
constexpr uint32_t FAT_EOF = 0x0FFFFFF8;
constexpr uint32_t FAT_EOC = 0x0FFFFFFF;
constexpr uint32_t FAT_BAD_CLUSTER = 0x0FFFFFF7;

struct FAT_Ent {
  uint32_t cluster : 28;
  uint32_t reserved : 4;

  bool eof() const {
    return cluster >= FAT_EOF;
  }
  bool eoc() const {
    return cluster == FAT_EOC;
  }
  bool bad() const {
    return cluster == FAT_BAD_CLUSTER;
  }
  bool free() const {
    return cluster == FAT_FREE;
  }
};
static_assert(sizeof(FAT_Ent) == 4);

enum class FILE_Attrs : uint8_t {
  ATTR_READ_ONLY = 0x01,
  ATTR_HIDDEN = 0x02,
  ATTR_SYSTEM = 0x04,
  ATTR_VOLUME_ID = 0x08,
  ATTR_DIRECTORY = 0x10,
  ATTR_ARCHIVE = 0x20,
  ATTR_LONG_NAME = ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID,
  ATTR_LONG_NAME_MASK = ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM |
                        ATTR_VOLUME_ID | ATTR_DIRECTORY | ATTR_ARCHIVE,
  MARK_AS_BITMASK_ENUM(ATTR_ARCHIVE),
};

constexpr uint8_t DIR_ENTRY_UNUSED = 0xE5;
constexpr uint8_t DIR_ENTRY_FREE = 0x00;

struct FAT32_DirEnt {
  union {
    char DIR_Name[11];
    struct {
      char file_name[8];
      char file_ext[3];
    };
  };
  FILE_Attrs DIR_Attr;
  char DIR_NTRes;
  uint8_t DIR_CrtTimeTenth;
  uint16_t DIR_CrtTime;
  uint16_t DIR_CrtDate;
  uint16_t DIR_LstAccDate;
  uint16_t DIR_FstClusHI;
  uint16_t DIR_WrtTime;
  uint16_t DIR_WrtDate;
  uint16_t DIR_FstClusLO;
  uint32_t DIR_FileSize;
  uint32_t FstClus() const {
    return (DIR_FstClusHI << 16) | DIR_FstClusLO;
  }
  bool unused() const {
    return (unsigned char)DIR_Name[0] == DIR_ENTRY_UNUSED;
  }
  void set_unused() {
    DIR_Name[0] = DIR_ENTRY_UNUSED;
  }
  bool end() const {
    return DIR_Name[0] == DIR_ENTRY_FREE;
  }
  bool valid() const {
    return not end() and not unused();
  }
  bool is_long_name() const {
    return (DIR_Attr & FILE_Attrs::ATTR_LONG_NAME_MASK) ==
           FILE_Attrs::ATTR_LONG_NAME;
  }
} __attribute__((__packed__));
