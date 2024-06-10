#include "fs/fat32fs.hpp"

#include "fs/log.hpp"
#include "io.hpp"

#define FS_TYPE "fat32fs"

namespace fat32fs {

bool FileSystem::init = false;

FileSystem::FileSystem() {
  if (not init) {
    init = true;
    sd_init();
  }

  block_buf = new char[BLOCK_SIZE];

  mbr = new MBR;
  sector_0_off = 0;
  read_data(0, mbr);

  if (not mbr->valid()) {
    FS_WARN("disk is invalid MBR (sig = %x%x)\n", mbr->sig[0], mbr->sig[1]);
    return;
  }

  if (mbr->entry[0].active) {
    FS_INFO("first partition entry is active\n");
  }

  if (mbr->entry[0].type != PARTITION_ID_FAT32_CHS) {
    FS_WARN("first partition entry isn't FAT32 with CHS (type = 0x%x)\n",
            mbr->entry[0].type);
    return;
  }

  khexdump(&mbr->entry[0], 16);

  FS_INFO("first sector 0x%x\n", mbr->entry[0].first_sector_LBA);
  auto print_CHS = [](const char* name, CHS chs) {
    FS_INFO("%s %u / %u / %u\n", name, chs.C, chs.H, chs.S);
  };
  print_CHS("first sector", mbr->entry[0].first_sector_CHS);
  print_CHS("last sector", mbr->entry[0].last_sector_CHS);

  sector_0_off = mbr->entry[0].first_sector_LBA;

  bpb = new FAT_BPB;
  read_data(0, bpb);

  strncpy(label, bpb->BS_VolLab, sizeof(bpb->BS_VolLab));
  FS_INFO("volumn label: '%s'\n", label);

  if (strncmp("FAT32   ", bpb->BS_FilSysType, sizeof(bpb->BS_FilSysType))) {
    FS_WARN("invalid type '%s'\n", bpb->BS_FilSysType);
    return;
  }

  if (bpb->BPB_RootEntCnt != 0) {
    FS_WARN("BPB_RootEntCnt is not zero (%d)\n", bpb->BPB_RootEntCnt);
    return;
  }

  RootDirSectors = ((bpb->BPB_RootEntCnt * 32) + (bpb->BPB_BytsPerSec - 1)) /
                   bpb->BPB_BytsPerSec;
  FS_INFO("RootDirSectors = %d\n", RootDirSectors);

  if (bpb->BPB_FATSz16 != 0) {
    FS_WARN("BPB_FATSz16 is not zero (%d)\n", bpb->BPB_FATSz16);
    return;
  }

  FATSz = bpb->BPB_FATSz32;

  read_block(bpb->BPB_RsvdSecCnt);
  kprintf("first FAT\n");
  khexdump(block_buf, BLOCK_SIZE);

  FirstDataSector =
      bpb->BPB_RsvdSecCnt + (bpb->BPB_NumFATs * FATSz) + RootDirSectors;
  FS_INFO("FirstDataSector = %d\n", FirstDataSector);

  if (bpb->BPB_TotSec16 != 0) {
    FS_WARN("BPB_TotSec16 is not zero (%d)\n", bpb->BPB_TotSec16);
    return;
  }

  TotSec = bpb->BPB_TotSec32;
  FS_INFO("TotSec = %d\n", TotSec);
  DataSec = TotSec -
            (bpb->BPB_RsvdSecCnt + (bpb->BPB_NumFATs * FATSz) + RootDirSectors);
  FS_INFO("DataSec = %d\n", DataSec);
  CountofClusters = DataSec / bpb->BPB_SecPerClus;
  FS_INFO("CountofClusters = %d\n", CountofClusters);

  if (CountofClusters < 4085) {
    FS_WARN("Volume is FAT12\n");
    return;
  } else if (CountofClusters < 65525) {
    FS_WARN("Volume is FAT16\n");
    return;
  } else {
    FS_INFO("Volume is FAT32\n");
  }

  fsinfo = new FAT32_FSInfo;
  read_data(1, fsinfo);

  if (not fsinfo->valid()) {
    FS_WARN("fsinfo is invalid (sig = 0x%x)\n", fsinfo->FSI_LeadSig);
    return;
  }

  read_block(cluster2sector(bpb->BPB_RootClus));
  kprintf("root cluster\n");
  khexdump(block_buf, BLOCK_SIZE);
}

};  // namespace fat32fs
