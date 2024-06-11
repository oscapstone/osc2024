#include "sd_driver.h"
#include "vfs.h"
#include "tmpfs.h"
#include "memory.h"
#include "shell.h"
#include "utils.h"
#include <stdint.h>

extern char* cpio_base;

// struct file_operations fat32_file_operations = {fat32_write,fat32_read,fat32_open,fat32_close};
// struct vnode_operations fat32_vnode_operations = {fat32_lookup,fat32_create,fat32_mkdir};

//reference: https://tw.easeus.com/data-recovery/fat32-disk-structure.html
struct PartitionEntry {
    uint8_t status;            // 磁碟區的當前狀態（00h=不活動，80h=活動）
    uint8_t start_head;        // 磁碟區開始 - 頭
    uint16_t start_sector_cylinder; // 磁碟區的開始 - 柱面/扇區
    uint8_t type;              // 磁碟區類型
    uint8_t end_head;          // 磁碟區結束 - 頭
    uint16_t end_sector_cylinder;   // 磁碟區結束 - 柱面/扇區
    uint32_t start_lba;        // MBR 與磁碟區中第一個扇區之間的扇區數
    uint32_t size_in_sectors;  // 磁碟區中的扇區數
} __attribute__((packed));

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
    uint8_t boot_code[356];
} __attribute__((packed));


struct fat32_mbr {
    struct FAT32BootSector boot_code;        // 可執行代碼（開機電腦）
    struct PartitionEntry partitions[4]; // 四個磁碟區條目
    uint16_t signature;            // 開機記錄簽名 (55AAh)
} __attribute__((packed));

// struct vnode * fat32_create_vnode(const char * name, char * data){
//     struct vnode * node = allocate_page(4096);
//     memset(node, 4096);
//     node -> f_ops = &fat32_file_operations;
//     node -> v_ops = &fat32_vnode_operations;
//     struct fat32_node * inode = allocate_page(4096);
//     memset(inode, 4096);
//     strcpy(name, inode -> name);
//     node -> internal = inode;
//     return node;
// }

int fat32_mount(struct filesystem *_fs, struct mount *mt){
    //initialize all entries
    mt -> fs = _fs;
    char buf[512];
    readblock(0, buf);
    struct fat32_mbr * mbr = buf;
    uart_puts("[Boot] Partition Status:\n\r");
    
    for(int i=0; i<4; i++){
        uart_puts("Partition ");
        uart_int(i+1);
        uart_puts(": ");
        uart_int(mbr -> partitions[i].status);
        newline();
    }
    //set root
    // const char * fname = "fat32";
    // mt -> root = fat32_create_vnode(fname, 0); //1: directory, 2: mount
    // strcpy("fat32", ((struct fat32_node *)mt -> root -> internal) -> name);

    // //create all entries for files
    // struct fat32_node * inode = mt -> root -> internal;
    // struct cpio_newc_header *fs = (struct cpio_newc_header *)cpio_base;
    // char *current = (char *)cpio_base;
    // int idx = 0;
    return 0;
}

int reg_fat32(){
    struct filesystem fs;
    fs.name = "fat32";
    fs.setup_mount = fat32_mount;
    return register_filesystem(&fs);
}

// int fat32_write(struct file *file, const void *buf, size_t len){
//     return -1;
// }

// int fat32_read(struct file *file, void *buf, size_t len){
//     struct fat32_node * internal = file -> vnode -> internal;
//     if(file -> f_pos + len > internal -> size)
//         len = internal -> size - file -> f_pos;
//     for(int i=0; i<len;i++){
//         ((char *)buf)[i] = internal -> data[i + file -> f_pos];
//     }
//     file -> f_pos += len;
//     return len;
// }

// int fat32_open(struct vnode *file_node, struct file **target){
//     (*target) -> vnode = file_node;
//     (*target) -> f_ops = file_node -> f_ops;
//     (*target) -> f_pos = 0;
//     return 0;
// }

// int fat32_close(struct file *file){
//     free_page(file);
//     return 0;
// }

// int fat32_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name){
//     int idx = 0;
//     struct fat32_node * internal = dir_node -> internal;
//     while(internal -> entry[idx]){
//         struct fat32_node * entry_node = internal -> entry[idx] -> internal;
//         if(strcmp(entry_node -> name, component_name) == 0){
//             *target = internal -> entry[idx];
//             return 0;
//         }
//         idx++;
//     }
//     return -1;
// }

// int fat32_create(struct vnode *dir_node, struct vnode **target, const char *component_name){
//     return -1;
// }

// int fat32_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name){
//     return -1;
// }