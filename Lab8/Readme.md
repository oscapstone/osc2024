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

## For QEMU Demo
To justify the result of fat_w, use mt.sh to mount img on vm.

## TODOs
* Question: why only QEMU
* Complete DEMO doc
* Basic2 comments and finalize
* finalize all code (maybe a version with all fucntions)