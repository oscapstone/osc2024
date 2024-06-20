#ifndef __FAT32_H__
#define __FAT32_H__

#include "vfs.h"
#include "stdint.h"

#ifndef DEFAULT_INODE_SIZE
#define DEFAULT_INODE_SIZE             (4096)
#endif

/**
 * File attribute in FAT32
 * https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#Directory_entry
 */
#define ATTR_READ_ONLY                  (0x01)
#define ATTR_HIDDEN                     (0x02)
#define ATTR_SYSTEM                     (0x04)
#define ATTR_VOLUME_ID                  (0x08)
#define ATTR_DIRECTORY                  (0x10)
#define ATTR_ARCHIVE                    (0x20)
#define ATTR_DEVICE                     (0x40)
#define ATTR_RESERVED                   (0x80)
#define ATTR_LONG_NAME                  (0x0f)

/* define file name type */
#define SFN                             (0x0b) // used in partition entry in MBR
#define LFN                             (0x0c)

#define FAT_ENTRY_MASK                  (0x0fffffff) // mask for FAT32 entry in FAT table, upper 4 bits are reserved.

#define DIR_ENTRY_SIZE                  (32) // size of directory entry in FAT32

/** 
 * https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#BPB20
 * Common structure of the Boot Sector used by most FAT versions for IBM compatible x86 machines since DOS 2.0
 * https://lexra.pixnet.net/blog/post/303910876
 */
struct fat_boot_sector {
    uint8_t jump_to_bootstrap[3];
    uint8_t oem_name[8];
    // BIOS Parameter Block
    uint16_t bytes_per_sector; // 512, 1024 ...
    uint8_t sectors_per_cluster;
    uint16_t nr_reserved_sectors; // FAT32 uses 32
    uint8_t nr_fat_copies;
    uint16_t nr_root_entry; // 0 for FAT32
    uint16_t nr_sectors_fat16; // total number of sectors in the file system, in case partition < 32 Mb
    uint8_t media_descripter_type;
    uint16_t nr_sectors_per_fat; // 0 for FAT32 
    // DOS 3.31 BPB
    uint16_t nr_sectors_per_track;
    uint16_t nr_heads;
    uint32_t nr_hidden_sectors; // Default Boot Sector ends here
    uint32_t nr_sectors_fat32; // total number of sectors in FAT32
    // FAT32 Extended BPB
    uint32_t sectors_per_fat; // sectors per FAT table.
    uint16_t mirror_flags;
    uint16_t fs_version;
    uint32_t root_dir; // first cluster of root directory, usually 2
    uint16_t fs_info; // file system information sector number, usually 1.
    uint16_t backup_boot_sector; // backup boot sector location, usually 6. 0 or 0xffff if none.
    uint8_t reserved[12];
    uint8_t drv_num; // logical driver number
    uint8_t reserved1;
    uint8_t ext_sig; // extended signature
    uint32_t sn_partition; // serial number of partition
    uint8_t volume_label[11];
    uint8_t fs_type[8]; // file system type: FAT32
} __attribute__((packed));

/**
 * https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#Directory_entry
 * Directory entry
 * https://blog.csdn.net/ZCShouCSDN/article/details/97610903
*/
struct sfn_entry {
    uint8_t name[8]; // Meet ' ' or 0x00, stop.
    uint8_t file_ext[3];
    uint8_t attr; // file attr, if 0x0f, it is LFN
    uint8_t ntres; // NTRes: Optional flags that indicates case information of the SFN.
    uint8_t crt_time_tenth; // CrtTimeTenth
    uint16_t crt_time;
    uint16_t crt_date;
    uint16_t lst_acc_date; //Optional last accesse date.
    uint16_t cluster_high; // Upper part of cluster number.
    uint16_t wrt_time;
    uint16_t wrt_date;
    uint16_t cluster_low; // Lower part of cluster number
    uint32_t file_size;
} __attribute__((packed));

/**
 * https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#Directory_table
 * VFAT long file names
*/
struct lfn_entry {
    uint8_t sequence_nr;
    uint8_t name1[10];
    uint8_t attr;
    uint8_t type;
    uint8_t checksum;
    uint8_t name2[12];
    uint16_t first_cluster;
    uint8_t name3[4]; // name character
} __attribute((packed));

/* fat32 file system internal node */
struct fat32_inode {
    enum fsnode_type type;
    // char name[MAX_PATH_LEN];
    char name[MAX_FILE_NAME_LEN];
    struct vnode *childs[MAX_DIR_NUM];
    char *data;
    unsigned long data_size;
    unsigned int cluster; // relative cluster number (for FAT table usage, can't use absolute cluster number)
    unsigned int abs_block_num; // absolute block number = data_region + (cluster - 2) * blocks_per_cluster
};

struct fat32_info {
    unsigned int starting_sector; // also the boot sector block number.
    unsigned int fat_region; // FAT region starting block number.
    unsigned int data_region; // also root dir starting block number.

    unsigned int blocks_per_cluster;

    unsigned char file_name_type; // 0x0b for SFN, 0x0c for LFN
};

extern struct filesystem fat32_filesystem;
extern struct fat_boot_sector fat_boot_sector;
extern struct fat32_info fat_info;

struct vnode *fat32_create_vnode(struct mount *mount, enum fsnode_type type);
void get_fat32_boot_sector(void);
void print_fat32_boot_sector(void);

#endif // __FAT32_H__