#ifndef _FAT32_H
#define _FAT32_H

#include "sd_card.h"
#include "types.h"
#include "vfs.h"

#define SECTOR_SIZE 512
#define DIRENT_SIZE 32
#define SFN_ATTR 0x20
#define DIR_ATTR 0x10

#define FREE_CLUSTER 0x0000000
#define EOC 0xFFFFFFF  // Last cluster in file
#define N_ENTRY_PER_FAT 128

// https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#BPB20
// https://www.easeus.com/resource/fat32-disk-structure.html
typedef struct fat32_bootsector {
  // DOS 2.0 BPB
  uint8_t jumpcode[3];  // Jump instruction
  uint8_t oem_name[8];  // determines in which system the disk was formatted.
  uint16_t bytes_per_sector;
  uint8_t n_sectors_per_cluster;
  uint16_t n_reserved_sectors;  // #logical sectors before the first FAT
  uint8_t n_fats;               // number of copies of FAT
  uint16_t n_root_dir_entries;
  uint16_t n_logical_sectors;
  uint8_t media_descriptor;
  uint16_t logical_sectors_per_fat;

  // DOS 3.31 BPB
  uint16_t physical_sectors_per_track;
  uint16_t n_heads;           // number of heads for disks
  uint32_t n_hidden_sectors;  // count of hidden sectors preceding the partition
                              // hat contains this FAT volume
  uint32_t n_sectors;         // total logical sectors

  // DOS 7.1 Extended BPB for FAT32
  uint32_t n_sectors_per_fat;
  uint16_t mirror_flag;
  uint16_t fat32_drive_version;
  uint32_t root_dir_cluster_idx;    // Cluster number of root directory start,
                                    // typically 2 (first cluster)
  uint16_t fs_info_sector_num;      // Logical sector number of FS Information
                                    // Sector, typically 1,
  uint16_t backup_boot_sector_num;  // First logical sector number of a copy of
                                    // the three FAT32 boot sectors, typically 6
  uint8_t rsv[12];
  uint8_t physical_drive_num;
  uint8_t unused;
  uint8_t ext_boot_signature;
  uint32_t volume_id;
  uint8_t volumne_label[11];
  uint8_t fs_type[8];
  uint8_t executable_code[420];
  uint8_t boot_record_signature[2];
} __attribute__((packed)) fat32_bootsector;

typedef struct fat32_dirent_sfn {  // Short File Names (SFN)
  char name[8];
  char ext[3];
  uint8_t attr;               // File attributes
  uint8_t rsv;                // Reserved, must be 0
  uint8_t crtTimeTenth;       // Creation time in tenths of a second
  uint16_t crtTime;           // Creation time
  uint16_t crtDate;           // Creation date
  uint16_t lastAcccessDate;   // Last access date
  uint16_t firstClusterHigh;  // high 2 bytes of first cluster number in FAT32
  uint16_t wrtTime;           // last modified time
  uint16_t wrtDate;           // last modified date
  uint16_t firstClusterLow;  // first cluster in FAT12 and FAT16. low 2 bytes of
                             // first cluster in FAT32.
  uint32_t fileSize;         // file size in bytes
} __attribute__((packed)) fat32_dirent_sfn;

// typedef struct fat32_dirent_lfn {  // Long File Names (SFN)
//   uint8_t seq_num;    // Sequence number and flags (bit 6: last LFN entry,
//                       // bits 5-0: LFN order)
//   uint16_t name1[5];  // Characters 1-5 of the LFN (first 5 Unicode
//   characters) uint8_t attr;       // Attribute (always 0x0F for LFN) uint8_t
//   type;       // Type (always 0x00 for LFN) uint8_t checksum;   // Checksum
//   of the short filename uint16_t name2[6];  // Characters 6-11 of the LFN
//   (next 6 Unicode characters) uint16_t cluster_start;  // First cluster
//   (always 0x0000 for LFN) uint16_t name3[2];  // Characters 12-13 of the LFN
//   (last 2 Unicode characters)
// } __attribute__((packed)) fat32_dirent_lfn;

typedef struct fat32_metadata {
  uint32_t fat_region_block_idx;
  uint32_t data_region_block_idx;
  uint32_t root_dir_cluster_idx;
  uint32_t n_sectors;
  uint32_t n_sectors_per_fat;
  uint8_t n_sectors_per_cluster;
  uint8_t n_fats;
} fat32_metadata;

typedef struct fat32_internal {
  uint32_t first_cluster;
  uint32_t dirent_cluster;
  uint32_t size;
} fat32_internal;

typedef struct fat32_cache_block {
  struct fat32_cache_block* next;
  uint32_t block_idx;
  uint8_t block[BLOCK_SIZE];
  uint8_t dirty_flag;
} fat32_cache_block;

filesystem* fat32fs_init();
void fat32fs_init_metadata(mbr_partition_entry* partition_entry,
                           fat32_bootsector* fat32_bootsec);
#endif