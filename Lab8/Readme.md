# OSC2024 Lab8

## FAT32 Data Structures
### MBR
Master Boot Record, get partition entries.
```
struct fat32_mbr {
    char boot_code[446];
    struct PartitionEntry partitions[4]; // four partitions
    uint16_t signature; // (55AAh)
} __attribute__((packed));
```
Note: use packed to prevent automatic memory alignment
### PartitionEntry
```
struct PartitionEntry {
    uint8_t status;            // 00 no use, 80 active
    uint8_t start_head;       
    uint16_t start_sector_cylinder;
    uint8_t type;              
    uint8_t end_head;         
    uint16_t end_sector_cylinder;  
    uint32_t start_lba; // sectors to partition 
    uint32_t size_in_sectors; 
} __attribute__((packed));
```


### Boot Sector
The first sector of the partition.
```
struct FAT32BootSector {
    uint8_t jump_boot[3];
    uint8_t oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t num_fats;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t media;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t fs_type[8];
} __attribute__((packed));
```

## Directory Entry
To show the files in root directory
```
struct DirectoryEntry {
    uint8_t name[11];
    uint8_t attr;
    uint8_t nt_res;
    uint8_t crt_time_tenth;
    uint16_t crt_time;
    uint16_t crt_date;
    uint16_t lst_acc_date;
    uint16_t fst_clus_hi;
    uint16_t wrt_time;
    uint16_t wrt_date;
    uint16_t fst_clus_lo;
    uint32_t file_size;
} __attribute__((packed));
```


## For QEMU Demo
To justify the result of fat_w, use mt.sh to mount img on vm.