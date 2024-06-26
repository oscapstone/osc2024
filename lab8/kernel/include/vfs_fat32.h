#ifndef FAT32_H
#define FAT32_H

#include "u_list.h"
#include "vfs.h"

#define BLOCK_SIZE 512
#define FAT32_MAX_FILENAME 8
#define FAT32_MAX_EXTENSION 3

#define FAT32_FREE_CLUSTER 0x00000000
#define FAT32_END_OF_CHAIN 0x0FFFFFF8

// #define CLUSTER_ENTRY_PER_BLOCK (BLOCK_SIZE / sizeof(struct cluster_entry_t))
// #define DIR_PER_BLOCK (BLOCK_SIZE / sizeof(struct dir_t))
// #define INVALID_CID 0x0ffffff8 // Last cluster in file (EOC), reserved by FAT32

static unsigned int *file_allocation_table;
static struct fs_info *fs_info;
struct partition_t
{
    unsigned char status;
    unsigned char chss_head;
    unsigned char chss_sector;
    unsigned char chss_cylinder;
    unsigned char type;
    unsigned char chse_head;
    unsigned char chse_sector;
    unsigned char chse_cylinder;
    unsigned int lba;
    unsigned int sectors;
} __attribute__((packed));

struct MBR
{
    char bootstrap_code[0x1BE];            // 引導代碼區塊
    struct partition_t partition_table[4]; // 分區表，共4個分區
    unsigned short signature;              // MBR簽名，0x55AA
} __attribute__((packed));

struct boot_sector_t
{
    unsigned char jmpboot[3];
    unsigned char oemname[8];
    unsigned short bytes_per_sector;
    unsigned char sector_per_cluster;   // 每簇扇區數
    unsigned short reserved_sector_cnt; // 保留扇區數
    unsigned char fat_cnt;              // FAT 表數量，通常為 2
    unsigned short root_entry_cnt;
    unsigned short old_sector_cnt;
    unsigned char media;
    unsigned short sector_per_fat16;
    unsigned short sector_per_track;
    unsigned short head_cnt;
    unsigned int hidden_sector_cnt; // 隱藏扇區數，文件系統開始前的扇區數
    unsigned int sector_cnt;        // FAT32 definition
    unsigned int sector_per_fat32;  // 每個 FAT 表的扇區數
    unsigned short extflags;
    unsigned short ver;
    unsigned int root_cluster;
    unsigned short info; // 文件系統信息扇區號
    unsigned short bkbooksec;
    unsigned char reserved[12];
    unsigned char drvnum;
    unsigned char reserved1;
    unsigned char bootsig;
    unsigned int volid;
    unsigned char vollab[11];
    unsigned char fstype[8];
} __attribute__((packed));

// FAT32 FSINFO structure
struct fs_info
{
    unsigned int lead_signature;      // Should be 0x41615252
    unsigned char reserved1[480];     // Must be zero
    unsigned int structure_signature; // Should be 0x61417272
    unsigned int free_count;          // Number of free clusters; -1 if unknown
    unsigned int next_free;           // Next free cluster; 0xFFFFFFFF if unknown
    unsigned char reserved2[12];      // Must be zero
    unsigned int trail_signature;     // Should be 0xAA550000
} __attribute__((packed));

// attr of dir_t
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_LFN 0x0f
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_FILE_DIR_MASK (ATTR_DIRECTORY | ATTR_ARCHIVE)

// type of struct fat32_inode
#define FAT_DIR 1
#define FAT_FILE 2

struct fat32_inode
{
    struct boot_sector_t *boot_sector;
    struct sfn_file *sfn;
    char name[16];
    struct vnode *entry[16];
    // char *data;
    int data_block_cnt;
    size_t datasize;
};

struct sfn_file
{
    char name[FAT32_MAX_FILENAME];       // 文件名，最多8個字節，不足部分使用空格填充
    char extension[FAT32_MAX_EXTENSION]; // 文件擴展名，最多3個字節，不足部分使用空格填充
    char attribute;                      // 屬性
    char reserved;                       // 保留位
    char creation_time_tenth_seconds;    // 創建時間的1/10秒計數
    unsigned short creation_time;        // 創建時間
    unsigned short creation_date;        // 創建日期
    unsigned short last_access_date;     // 最後訪問日期
    unsigned short first_cluster_high;   // 文件起始簇高16位
    unsigned short last_write_time;      // 最後修改時間
    unsigned short last_write_date;      // 最後修改日期
    unsigned short first_cluster_low;    // 文件起始簇低16位
    unsigned int file_size;              // 文件大小（字節）
};

int register_fat32();

int fat32_mount(struct filesystem *fs, struct mount *mount);
int fat32_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int fat32_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int fat32_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);
int fat32_isdir(struct vnode *dir_node);
int fat32_getname(struct vnode *dir_node, const char **name);
int fat32_getsize(struct vnode *dir_node);

int fat32_write(struct file *file, const void *buf, size_t len);
int fat32_read(struct file *file, void *buf, size_t len);
int fat32_open(struct vnode *file_node, struct file **target);
int fat32_close(struct file *file);
long fat32_lseek64(struct file *file, long offset, int whence);
int fat32_sync(struct filesystem *fs);

void fat32_dump(struct vnode *vnode, int level);
void fat32_ls(struct vnode *vnode);
int fat32_op_deny();

static inline unsigned int get_first_cluster(struct sfn_file *entry)
{
    return (entry->first_cluster_high << 16) | entry->first_cluster_low;
}

static inline void fat32_get_file_name(char *name_array, struct sfn_file *entry)
{
    int idx = 0;
    for (int i = 0; i < FAT32_MAX_FILENAME; i++, idx++)
    {
        if (entry->name[i] == ' ')
        {
            break;
        }
        name_array[idx] = entry->name[i];
    }
    for (int i = 0; i < FAT32_MAX_EXTENSION; i++, idx++)
    {
        if (entry->extension[i] == ' ')
        {
            break;
        }
        else if (i == 0)
        {
            name_array[idx++] = '.';
        }
        name_array[idx] = entry->extension[i];
    }
    name_array[idx] = '\0';
}
#endif
