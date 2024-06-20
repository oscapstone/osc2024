#include "sd_driver.h"
#include "vfs.h"
#include "tmpfs.h"
#include "memory.h"
#include "shell.h"
#include "utils.h"
#include "fat32.h"
#include <stdint.h>

extern char* cpio_base;

struct file_operations fat32_file_operations = {fat32_write,fat32_read,fat32_open,fat32_close};
struct vnode_operations fat32_vnode_operations = {fat32_lookup,fat32_create,fat32_mkdir};

//reference #1: https://tw.easeus.com/data-recovery/fat32-disk-structure.html
struct PartitionEntry {
    uint8_t status;            // 00 no use, 80 active
    uint8_t start_head;       
    uint16_t start_sector_cylinder;
    uint8_t type;              
    uint8_t end_head;         
    uint16_t end_sector_cylinder;  
    uint32_t start_lba;        // sectors to partition
    uint32_t size_in_sectors; 
} __attribute__((packed));

//reference #2: https://lexra.pixnet.net/blog/post/303910876
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


struct fat32_mbr {
    char boot_code[446];
    struct PartitionEntry partitions[4]; // four partitions
    uint8_t signature1; // (55AAh)
    uint8_t signature2;
} __attribute__((packed));

// reference #3: https://www.codeguru.com/cplusplus/fat-root-directory-structure-on-floppy-disk-and-file-information/
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

struct fat32_node{
    char name[MAX_PATH];
    int type;
    struct vnode * entry[MAX_ENTRY];    
    char * data;
    uint32_t start_cluster;
    uint32_t size;
};

struct vnode * fat32_create_vnode(const char * name, int type, uint32_t start_cluster, uint32_t size){
    uart_puts("[FAT32] Create vnode for file ");
    uart_puts(name);
    newline();
    struct vnode * node = allocate_page(4096);
    memset(node, 4096);
    node -> f_ops = &fat32_file_operations;
    node -> v_ops = &fat32_vnode_operations;
    
    struct fat32_node * inode = allocate_page(4096);
    memset(inode, 4096);
    inode -> type = type;
    inode -> start_cluster = start_cluster;
    inode -> size = size;
    strcpy(name, inode -> name);
    node -> internal = inode;
    
    return node;
}

// https://en.wikipedia.org/wiki/8.3_filename
void format_filename(char *dest, const char *src) {
    int i, j;
    for (i = 0, j = 0; i < 11; i++) {
        if (i == 8) {
            dest[j++] = '.'; 
        }
        if (src[i] != ' ') {
            dest[j++] = src[i];
        }
    }
    dest[j] = '\0'; 
}

struct fat32_mbr * mbr;
int fat_start_block; //mbr -> partitions[0].start_lba;
struct FAT32BootSector * boot_sector; //need to use malloc
int root_dir_start_block; // 3588
uint32_t cluster_size; // 512
uint32_t entries_per_cluster; // 16
struct DirectoryEntry * rootdir; // read from start_block (need to use malloc)

// https://wiki.osdev.org/FAT
int fat32_mount(struct filesystem *_fs, struct mount *mt){
    //initialize partition entries, detail see reference #1
    //partition -> cluster(block) -> sector
    mt -> fs = _fs;
    char buf[512];
    readblock(0, buf);
    mbr = buf;
    uart_hex(buf[510]);
    newline();
    uart_hex(((char *)mbr)[510]);
    newline();
    uart_hex(mbr -> signature1);
    newline();
    fat_start_block = mbr -> partitions[0].start_lba; // get the first block of the partition
    readblock(fat_start_block, buf); // Read the first block of the partition as boot_sector
    boot_sector = allocate_page(4096);
    for(int i=0; i<512; i++){
        ((char *)boot_sector)[i] = buf[i];
    }

    cluster_size = boot_sector -> bytes_per_sector * boot_sector -> sectors_per_cluster; // 512 * 1
    entries_per_cluster = cluster_size / 32;
    
    /* #Reference 3: Out of this, the 0th sector (the first 512 bytes) is reserved as the boot sector. Next sectors are reserved for FATs.
       Immediately after these FAT sectors, the root directory sectors start. */
    root_dir_start_block = fat_start_block + ((boot_sector->reserved_sector_count + (boot_sector->num_fats * boot_sector->fat_size_32)) * boot_sector->sectors_per_cluster);
    readblock(root_dir_start_block, buf);

    //for root dir lookup
    rootdir = allocate_page(4096);
    for(int i=0; i<512; i++){
        ((char *)rootdir)[i] = buf[i];
    }
    
    mt -> root = fat32_create_vnode("fat32", 2, 0, 0);
    return 0;
}

int reg_fat32(){
    struct filesystem fs;
    fs.name = "fat32";
    fs.setup_mount = fat32_mount;
    return register_filesystem(&fs);
}

char* memcpy(void *dest, const void *src, unsigned long long len)
{
    char *d = dest;
    const char *s = src;
    while (len--)
        *d++ = *s++;
    return dest;
}

uint32_t find_free_cluster() {
    uint32_t total_clusters = boot_sector->total_sectors_32 / boot_sector->sectors_per_cluster;
    uint32_t fat_table_sector = fat_start_block + boot_sector->reserved_sector_count;
    uint32_t clusters_per_sector = boot_sector->bytes_per_sector / sizeof(uint32_t);
    uint32_t fat_entry;

    char buffer[512];  // Assuming sector size is 512 bytes

    // FAT table https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system
    for (uint32_t sector = 0; sector < boot_sector->fat_size_32; sector++) {
        readblock(fat_table_sector + sector, buffer);
        for (uint32_t entry = 0; entry < clusters_per_sector; entry++) {
            memcpy(&fat_entry, buffer + entry * sizeof(uint32_t), sizeof(uint32_t));
            if (fat_entry == 0) {
                return sector * clusters_per_sector + entry;
            }
        }
    }

    return 0;  // No free cluster found
}

void update_file_size(struct fat32_node *file_node) {
    char buf[512];
    readblock(root_dir_start_block, buf);
    struct DirectoryEntry *dir = (struct DirectoryEntry *)buf;
    char formatted_name[13];

    for (int i = 0; i < entries_per_cluster; i++) {
        if (dir[i].name[0] == 0x00) break; 
        if (dir[i].name[0] == 0xE5) continue;
        format_filename(formatted_name, dir[i].name);

        if (strcmp(formatted_name, file_node->name) == 0) {
            dir[i].file_size = file_node->size;
            writeblock(root_dir_start_block, buf);
            return;
        }
    }
}

int fat32_write(struct file *file, const void *buf, size_t len) {
    //find cluster and write block
    struct fat32_node *file_node = (struct fat32_node*) file->vnode->internal;
    uint32_t cluster_number = file_node->start_cluster;
    uint32_t sector_offset = file->f_pos / boot_sector->bytes_per_sector;
    uint32_t cluster_offset = sector_offset / boot_sector->sectors_per_cluster;
    uint32_t current_cluster = cluster_number;

    uint32_t sector_in_cluster = sector_offset % boot_sector->sectors_per_cluster;
    uint32_t byte_offset = file->f_pos % boot_sector->bytes_per_sector;

    char sector_buffer[512];
    uint32_t first_sector_of_cluster = root_dir_start_block + (current_cluster - 2) * boot_sector->sectors_per_cluster + sector_in_cluster;
    readblock(first_sector_of_cluster, sector_buffer);
    for(int i = 0; i<len; i++){
        //uart_send(((char *)buf)[i]);
        ((char*)sector_buffer)[i+byte_offset] = ((char *)buf)[i];
    }
    //newline();
    writeblock(first_sector_of_cluster, sector_buffer);

    file->f_pos += len;

    if (file->f_pos > file_node->size) {
        file_node->size = file->f_pos; 
        update_file_size(file_node);
    }
    // uart_int(file_node -> size);
    // newline();
    return len;
}

// https://www.win.tue.nl/~aeb/linux/fs/fat/fatgen103.pdf
int fat32_read(struct file* file, void* ret, uint64_t len) {
    struct fat32_node* file_node = (struct fat32_node*)file->vnode->internal;
    if(file_node -> size < len + file -> f_pos)
        len = (file_node -> size - file->f_pos);
    int cluster_number = file_node -> start_cluster;
    //FAT + the place of first sector(followed by FAT) + cluster offset (start by 2)
    uint32_t first_sector_of_cluster = root_dir_start_block + (cluster_number - 2) * boot_sector->sectors_per_cluster;

    char buffer[512];
    memset(buffer, 512);
    readblock(first_sector_of_cluster, (char*) buffer);
    for(int i = 0; i<len;i++){
        ((char *) ret)[i] = buffer[i + file -> f_pos];
    }

    file -> f_pos += len;
    return len;
}

int fat32_open(struct vnode *file_node, struct file **target){
    (*target) -> vnode = file_node;
    (*target) -> f_ops = file_node -> f_ops;
    (*target) -> f_pos = 0;
    return 0;
}

int fat32_close(struct file *file){
    free_page(file);
    return 0;
}

int fat32_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name){
    // check entry, if not in entry, check sd card, and if still no, return -1
    char buf[512];
    int idx = 0;
    struct fat32_node * internal = dir_node -> internal;
    while(internal -> entry[idx]){
        struct fat32_node * entry_node = internal -> entry[idx] -> internal;
        // uart_puts("First Level Lookup, filename: ");
        // uart_puts(entry_node -> name);
        // newline();
        if(strcmp(entry_node -> name, component_name) == 0){
            *target = internal -> entry[idx];
            return 0;
        }
        idx++;
    }

    struct DirectoryEntry *dir = rootdir;
    char formatted_name[13];
    int type = -1;

    //more detail see reference #3
    //search root directory and if found create new vnode
    for (int i = 0; i < entries_per_cluster; i++) { 
        if (dir[i].name[0] == 0x00) break; //available and no entry beyond has been used
        if (dir[i].name[0] == 0xE5) continue; //erased

        format_filename(formatted_name, dir[i].name); // change the name to XXX.XX format

        if(strcmp(formatted_name, component_name) == 0){
            uart_puts("[FAT32] Found ");
            uart_puts(formatted_name);
            
            if (dir[i].attr & 0x10) { //file
                type = 1;
            } else { //directory
                type = 3;
            }
            
            uint32_t start_cluster = ((uint32_t)dir[i].fst_clus_hi << 16) | (uint32_t)dir[i].fst_clus_lo; //high 16 bit and low 16 bit
            uint32_t file_size = dir[i].file_size;
            uart_puts(" with size ");
            uart_int(file_size);
            uart_puts(" in start cluster ");
            uart_int(start_cluster);
            newline();
            //set new vnode and assign to internal -> idx
            //set the start_cluster for read
            internal -> entry[idx] = fat32_create_vnode(formatted_name, type, start_cluster, file_size);
            *target = internal -> entry[idx];
            return 0;
        }
    }

    return -1;
}

void format_83_filename(char *dest, const char *src) {
    int i = 0, j = 0;
    int dotIndex = -1;

    while (src[i] != '\0') {
        if (src[i] == '.') {
            dotIndex = i;
        }
        i++;
    }

    for (i = 0; i < 11; i++) {
        dest[i] = ' ';
    }

    if (dotIndex == -1) {
        dotIndex = i;
        if (dotIndex > 8) dotIndex = 8;
    }

    for (i = 0; i < dotIndex && i < 8; i++) {
        dest[i] = (src[i] >= 'a' && src[i] <= 'z') ? src[i] - 32 : src[i];
    }

    if (src[dotIndex] == '.') {
        for (j = 0; j < 3 && src[dotIndex + 1 + j] != '\0'; j++) {
            dest[8 + j] = (src[dotIndex + 1 + j] >= 'a' && src[dotIndex + 1 + j] <= 'z') ? src[dotIndex + 1 + j] - 32 : src[dotIndex + 1 + j];
        }
    }
}

void write_directory_entry(struct DirectoryEntry *entry, uint32_t directory_start_block, int entry_index) {
    uint32_t entries_per_sector = boot_sector->bytes_per_sector / sizeof(struct DirectoryEntry);
    uint32_t sector_number = directory_start_block + (entry_index / entries_per_sector);
    uint32_t offset_in_sector = (entry_index % entries_per_sector) * sizeof(struct DirectoryEntry);

    char sector_buffer[512]; 
    readblock(sector_number, sector_buffer);
    memcpy(sector_buffer + offset_in_sector, entry, sizeof(struct DirectoryEntry));
    writeblock(sector_number, sector_buffer);
}


int fat32_create(struct vnode *dir_node, struct vnode **target, const char *component_name) {
    struct DirectoryEntry *dir = rootdir;
    int empty_index = -1;
    for (int i = 0; i < entries_per_cluster; i++) {
        if (dir[i].name[0] == 0x00 || dir[i].name[0] == 0xE5) { 
            empty_index = i;
            break;
        }
    }
    
    if (empty_index == -1) {
        return -1;
    }
    
    // look for empty dir
    uint32_t free_cluster = find_free_cluster();
    if (free_cluster == 0) {
        return -1; 
    }
    uart_puts("[FAT32] Create file in cluster ");
    uart_int(free_cluster);
    
    memset(&dir[empty_index], sizeof(struct DirectoryEntry));
    format_83_filename(dir[empty_index].name, component_name); 
    uart_puts(" with file name: ");
    uart_puts(dir[empty_index].name);
    newline();
    dir[empty_index].fst_clus_hi = (free_cluster >> 16) & 0xFFFF;
    dir[empty_index].fst_clus_lo = free_cluster & 0xFFFF;
    dir[empty_index].file_size = 0;
    dir[empty_index].attr = 0x20;
    write_directory_entry(&dir[empty_index], root_dir_start_block, empty_index);
    
    struct fat32_node * internal = dir_node -> internal;
    int idx = -1;

    for(int i=0; i<MAX_ENTRY; i++){
        if(internal -> entry[i] == 0){
            idx = i;
            break;
        }
        struct fat32_node* infile = internal -> entry[i] -> internal;
        if(strcmp(infile -> name, component_name) == 0){
            uart_puts("FILE EXISTED!!!\n\r");
            return -1;
        }
    }
    
    struct vnode * node = fat32_create_vnode(component_name, 3, free_cluster, 0);
    internal -> entry[idx] = node;
    *target = node;
    return 0;
}


int fat32_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name){
    return -1;
}