#ifndef __MBR_H__
#define __MBR_H__

#include "stdint.h"

#ifndef BLOCK_SIZE
#define BLOCK_SIZE                 (512)
#endif

#define MBR_BOOT_SIG               (0xaa55)

/* https://en.wikipedia.org/wiki/Partition_type */
#define PARTITION_TYPE_FAT32_LBA   (0x0c)
#define PARTITION_TYPE_FAT32_CHS   (0x0b)

struct partition_table_entry {
    uint8_t boot_indicator; // 80h active
    uint8_t starting_chs_valu[3];
    uint8_t partition_type; // 0x0c: FAT32 with LBA. https://en.wikipedia.org/wiki/Partition_type, 0x0b: FAT32 with CHS
    uint8_t ending_chs_value[3];
    uint32_t starting_sector; // The starting sector of the partition.
    uint32_t partition_size;
} __attribute__((packed));

/* https://en.wikipedia.org/wiki/Master_boot_record */
struct general_mbr {
    uint8_t bootstrap_code_area[446];
    struct partition_table_entry partitions[4];
    uint16_t boot_signature; // should be 0x55,0xAA, in little endian is 0xAA55.
};

extern struct general_mbr mbr;


void get_mbr(void);


#endif // __MBR_H__