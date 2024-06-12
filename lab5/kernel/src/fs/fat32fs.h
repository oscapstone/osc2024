#pragma once

#include "base.h"

#include "fs.h"

// reference: https://cscie92.dce.harvard.edu/spring2024/slides/FAT32%20File%20Structure.pdf

// only support SFN fat32
#define FAT32_FS_NAME           "fat32"

FS_FILE_SYSTEM* fat32fs_create();

#define FAT32_TYPE_NAME         "FAT32   "
#define FAT32_TYPE_NAME_LEN     8

typedef struct PACKED {
    U8 jmp[3];              // the jump code for booting
    U8 oem[8];
    U16 bytes_per_sector;
    U8 sectors_per_cluster;
    U16 reserved_sectors;
    U8 num_fat;                     // how many fat table
    U16 max_root_entries;           // maximum root directory entries
    U16 total_sectors_fat16;        // (N/A for FAT32) 
    U8 media;
    U16 sectors_per_fat16;          // (N/A for FAT32)
    U16 sectors_per_track;
    U16 num_heads;
    U32 num_hidden_sectors;
    U32 num_sectors;                // Number of sectors in partition
    U32 sectors_per_fat32;          // how many sector for FAT table entry
    U16 flags;
    U16 version;
    U32 root_cluster;
    U16 info_sector;
    U16 backup_sector;
    U8 reserved[12];
    U8 drive_number;                // logical drive number of partition
    U8 unused;
    U8 signature;                   // 0x29
    U32 vol_id;
    U8 vol_label[11];               // your partition label
    char fs_type[8];                  // the label "FAT32   "
} FAT32_BPB;

typedef struct {
    char signature[4];
    char reserved[480];
    char structure_signature[4];
    U32 free_count;
    U32 next_free;
    char reserved2[12];
}FAT32_FS_INFO_RAW;

typedef struct 
{
    
}FAT32_FS_INFO;


typedef struct
{
    FAT32_BPB bpb;
    U32 fat_start_sector;           // the start sector of FAT table
    U32 data_start_sector;          // the start sector of data
    U32 sector_per_fat;
    U16 bytes_per_sector;
    /**
     * The FAT table is 32 bit entry but only use 28 bit
    */
    U32* fat_table;                // read the fat table and store in memory
    U32 fat_table_size;             // the size of the FAT table
    U32 next_free_cluster;          // the next free cluster
} FAT32_FS_INTERNAL;

typedef struct 
{
    U32 cluster_id;
    U32 num_block;          // in block
    U32 parent_dir_entry_index;     // the index of the parent dir entry
    BOOL is_dirty;          // whether this node has been modify(write)
    BOOL is_loaded;         // whether this node has been content loaded.
}FAT32_VNODE_INTERNAL;

// fat table symbol
#define FAT32_FAT_FREE        0
#define FAT32_FAT_BAD         0x0ffffff7
#define FAT32_FAT_EOF_START   0x0ffffff8
#define FAT32_FAT_EOF_END     0x0FFFFFFF

#define FAT_ENTRY_ALLOCATED_AND_END_OF_FILE FAT32_FAT_EOF_END

// the first byte of the dir entry name
#define FAT32_DIR_ENTRY_LAST_AND_UNUSED 0x0
#define FAT32_DIR_ENTRY_UNUSED 0xE5


// SFN format, 32 bytes
typedef struct PACKED {
    char name[8];           // file name
    char ext[3];            // ext name
    U8 attribute;
    U8 nt_res;
    U8 creation_time_tenth;   // millisecond part of creation time
    U16 creation_time;
    U16 creation_date;
    U16 last_access_date;
    U16 high_block_index;       // high cluster index
    U16 write_time;
    U16 write_date;
    U16 low_block_index;        // low cluster index
    U32 file_size;
}FAT32_DIR_ENTRY;


#define FAT32_DIR_ENTRY_ATTR_SYS        0x04
#define FAT32_DIR_ENTRY_ATTR_DIR        0x10
#define FAT32_DIR_ENTRY_ATTR_LONG_NAME  0x0f
