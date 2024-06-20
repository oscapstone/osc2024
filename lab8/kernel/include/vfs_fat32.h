#ifndef FAT32_H
#define FAT32_H

#include "u_list.h"
#include "vfs.h"

#define BLOCK_SIZE 512
#define CLUSTER_ENTRY_PER_BLOCK (BLOCK_SIZE / sizeof(struct cluster_entry_t))
#define DIR_PER_BLOCK (BLOCK_SIZE / sizeof(struct dir_t))
#define INVALID_CID 0x0ffffff8 // Last cluster in file (EOC), reserved by FAT32

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

struct boot_sector_t
{
    unsigned char jmpboot[3];
    unsigned char oemname[8];
    unsigned short bytes_per_sector;
    unsigned char sector_per_cluster;
    unsigned short reserved_sector_cnt; // red
    unsigned char fat_cnt;              // yellow
    unsigned short root_entry_cnt;
    unsigned short old_sector_cnt;
    unsigned char media;
    unsigned short sector_per_fat16;
    unsigned short sector_per_track;
    unsigned short head_cnt;
    unsigned int hidden_sector_cnt; // blue
    unsigned int sector_cnt;        // FAT32 definition
    unsigned int sector_per_fat32;  // pink
    unsigned short extflags;
    unsigned short ver;
    unsigned int root_cluster;
    unsigned short info;
    unsigned short bkbooksec;
    unsigned char reserved[12];
    unsigned char drvnum;
    unsigned char reserved1;
    unsigned char bootsig;
    unsigned int volid;
    unsigned char vollab[11];
    unsigned char fstype[8];
} __attribute__((packed));

struct fat_info_t
{
    struct boot_sector_t bs;  // boot_sector from MBR Partition Table
    unsigned int fat_lba;     // FAT  Region in FAT   fs Definition
    unsigned int cluster_lba; // Data Region in FAT32 fs Definition
};

// attr of dir_t
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_LFN 0x0f
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_FILE_DIR_MASK (ATTR_DIRECTORY | ATTR_ARCHIVE)

// Directory structure for FAT SFN
// http://elm-chan.org/docs/fat_e.html
struct dir_t
{
    unsigned char name[11];
    unsigned char attr;
    unsigned char ntres;
    unsigned char crttimetenth;
    unsigned short crttime;
    unsigned short crtdate;
    unsigned short lstaccdate;
    unsigned short ch;
    unsigned short wrttime;
    unsigned short wrtdate;
    unsigned short cl;
    unsigned int size;
} __attribute__((packed));

// Directory structure for FAT LFN
// https://blog.csdn.net/ZCShouCSDN/article/details/97610903
struct long_dir_t
{
    unsigned char order;
    unsigned char name1[10];
    unsigned char attr;
    unsigned char type;
    unsigned char checksum;
    unsigned char name2[12];
    unsigned short fstcluslo;
    unsigned char name3[4];
} __attribute__((packed));

struct cluster_entry_t
{
    union
    {
        unsigned int val;
        struct
        {
            unsigned int idx : 28;
            unsigned int reserved : 4;
        };
    };
};

struct filename_t
{
    union
    {
        unsigned char fullname[256];
        struct
        {
            unsigned char name[13];
        } part[20];
    };
} __attribute__((packed));

struct fat_file_block_t
{
    /* Link fat_file_block_t */
    struct list_head list;
    unsigned int oid; // offset if of file, N = offset N* BLOCK_SIZE of file
    unsigned int cid; // cluster id
    unsigned int bufIsUpdated;
    unsigned int isDirty;
    unsigned char buf[BLOCK_SIZE];
};

struct fat_file_t
{
    /* Head of fat_file_block_t chain */
    struct list_head list;
    unsigned int size;
};

struct fat_dir_t
{
    /* Head of fat32_inode chain */
    struct list_head list;
};

struct fat_mount_t
{
    /* Link fat_mount_t */
    struct list_head list;
    struct mount *mount;
};

// type of struct fat32_inode
#define FAT_DIR 1
#define FAT_FILE 2

struct fat32_inode
{
    const char *name;
    struct vnode *node; // redirect to vnode
    struct fat_info_t *fat;
    /* Link fat32_inode */
    struct list_head list;
    /* cluster id */
    unsigned int cid; // cluster id
    unsigned int type;
    union
    {
        struct fat_dir_t *dir;
        struct fat_file_t *file;
    };

    struct boot_sector_t *boot_sector;
    struct sfn_file *sfn;
    char Name[16];
    struct vnode *entry[16];
    char *data;
    size_t datasize;
};

struct sfn_file
{
	char name[11];					 // 檔案名稱
	char attribute;					 // 屬性
	char reserved;					 // 保留位
	char creation_time_tenth_seconds; // 創建時間的1/10秒計數
	unsigned short creation_time;				 // 創建時間
	unsigned short creation_date;				 // 創建日期
	unsigned short last_access_date;			 // 最後訪問日期
	unsigned short first_cluster_high;		 // 文件起始簇高16位
	unsigned short last_write_time;			 // 最後修改時間
	unsigned short last_write_date;			 // 最後修改日期
	unsigned short first_cluster_low;			 // 文件起始簇低16位
	unsigned int file_size;					 // 文件大小（字節）
};

int
register_fat32();

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

unsigned int alloc_cluster(struct fat_info_t *fat, unsigned int prev_cid);
unsigned int get_next_cluster(unsigned int fat_lba, unsigned int cluster_id);

struct vnode *_create_vnode(struct vnode *parent, const char *name, unsigned int type, unsigned int cid, unsigned int size);

int _lookup_cache(struct vnode *dir_node, struct vnode **target, const char *component_name);
struct dir_t *__lookup_fat32(struct vnode *dir_node, const char *component_name, unsigned char *buf, int *buflba);
int _lookup_fat32(struct vnode *dir_node, struct vnode **target, const char *component_name);

int _readfile(void *buf, struct fat32_inode *data, unsigned long long fileoff, unsigned long long len);
int _readfile_fat32(struct fat32_inode *data, unsigned long long bckoff, unsigned char *buf, unsigned int bufoff, unsigned int size, unsigned int oid, unsigned int cid);
int _readfile_cache(struct fat32_inode *data, unsigned long long bckoff, unsigned char *buf, unsigned long long bufoff, unsigned int size, struct fat_file_block_t *block);
int _readfile_seek_fat32(struct fat32_inode *data, unsigned int foid, unsigned int fcid, struct fat_file_block_t **block);
int _readfile_seek_cache(struct fat32_inode *data, unsigned int foid, struct fat_file_block_t **block);

int _writefile(const void *buf, struct fat32_inode *data, unsigned long long fileoff, unsigned long long len);
int _writefile_fat32(struct fat32_inode *data, unsigned long long bckoff, const unsigned char *buf, unsigned int bufoff, unsigned int size, unsigned int oid, unsigned int cid);
int _writefile_cache(struct fat32_inode *data, unsigned long long bckoff, const unsigned char *buf, unsigned long long bufoff, unsigned int size, struct fat_file_block_t *block);
int _writefile_seek_fat32(struct fat32_inode *data, unsigned int foid, unsigned int fcid, struct fat_file_block_t **block);
int _writefile_seek_cache(struct fat32_inode *data, unsigned int foid, struct fat_file_block_t **block);

void _sync_dir(struct vnode *dirnode);
void _do_sync_dir(struct vnode *dirnode);
void _do_sync_file(struct vnode *filenode);

void fat32_dump(struct vnode *vnode, int level);
void fat32_ls(struct vnode *vnode);
int fat32_op_deny();
#endif
