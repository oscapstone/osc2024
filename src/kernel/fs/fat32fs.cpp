#include "fs/fat32fs.hpp"

#include "fs/fat.hpp"
#include "fs/log.hpp"
#include "fs/mbr.hpp"
#include "fs/sdhost.hpp"
#include "io.hpp"

#define FS_TYPE "fat32fs"

namespace fat32fs {

::Vnode* FileSystem::root = nullptr;
bool FileSystem::init = false;

FileSystem::FileSystem() {
  if (init)
    return;
  init = true;

  sd_init();

  char buf[BLOCK_SIZE];

  readblock(0, buf);
  auto mbr = new MBR;
  memcpy(mbr, buf, sizeof(MBR));

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

  klog("first sector 0x%x\n", mbr->entry[0].first_sector_LBA);
  klog("first sector %u / %u / %u\n", mbr->entry[0].first_sector_CHS.C,
       mbr->entry[0].first_sector_CHS.H, mbr->entry[0].first_sector_CHS.S);
  klog("last sector %u / %u / %u\n", mbr->entry[0].last_sector_CHS.C,
       mbr->entry[0].last_sector_CHS.H, mbr->entry[0].last_sector_CHS.S);

  auto sector_0_off = mbr->entry[0].first_sector_LBA;
  auto read_block = [&](uint32_t id) { readblock(sector_0_off + id, buf); };

  read_block(0);
  auto bpb = new FAT_BPB;
  memcpy(bpb, buf, sizeof(FAT_BPB));

  char label[12]{};
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

  auto RootDirSectors =
      ((bpb->BPB_RootEntCnt * 32) + (bpb->BPB_BytsPerSec - 1)) /
      bpb->BPB_BytsPerSec;
  FS_INFO("RootDirSectors = %d\n", RootDirSectors);

  if (bpb->BPB_FATSz16 != 0) {
    FS_WARN("BPB_FATSz16 is not zero (%d)\n", bpb->BPB_FATSz16);
    return;
  }

  auto FATSz = bpb->BPB_FATSz32;

  read_block(bpb->BPB_RsvdSecCnt);
  kprintf("first FAT\n");
  khexdump(buf, BLOCK_SIZE);

  auto FirstDataSector =
      bpb->BPB_RsvdSecCnt + (bpb->BPB_NumFATs * FATSz) + RootDirSectors;
  FS_INFO("FirstDataSector = %d\n", FirstDataSector);

  auto get_first_sector = [&](uint32_t cluster_N) {
    return ((cluster_N - 2) * bpb->BPB_SecPerClus) + FirstDataSector;
  };

  if (bpb->BPB_TotSec16 != 0) {
    FS_WARN("BPB_TotSec16 is not zero (%d)\n", bpb->BPB_TotSec16);
    return;
  }

  auto TotSec = bpb->BPB_TotSec32;
  FS_INFO("TotSec = %d\n", TotSec);
  auto DataSec = TotSec - (bpb->BPB_RsvdSecCnt + (bpb->BPB_NumFATs * FATSz) +
                           RootDirSectors);
  FS_INFO("DataSec = %d\n", DataSec);
  auto CountofClusters = DataSec / bpb->BPB_SecPerClus;
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

  read_block(1);
  auto fsinfo = new FAT32_FSInfo;
  memcpy(fsinfo, buf, sizeof(FAT32_FSInfo));

  if (not fsinfo->valid()) {
    FS_WARN("fsinfo is invalid (sig = 0x%x)\n", fsinfo->FSI_LeadSig);
    return;
  }

  read_block(FirstDataSector);
  kprintf("first data\n");
  khexdump(buf, BLOCK_SIZE);
}

};  // namespace fat32fs
