#ifndef FAT32_H
#define FAT32_H

#include "list.h"
#include "vfs.h"

#define BLOCK_SIZE 512
#define CLUSTER_ENTRY_PER_BLOCK (BLOCK_SIZE / sizeof(struct cluster_entry_t))
#define DIR_PER_BLOCK (BLOCK_SIZE / sizeof(struct dir_t))
#define INVALID_CID 0x0ffffff8  // Last cluster in file (EOC), reserved by FAT32

// from https://thestarman.pcministry.com/asm/mbr/PartTables.htm
// A Partition Table Entry Table 4
struct partition_t {                // Bytes    Content
    unsigned char status;           // 0        Boot Indicator (80h = active)
                                    // 1 - 3    Starting CHS values
    unsigned char chss_head;        // 1        Starting Head
    unsigned char chss_sector;      // 2        Starting Sector
    unsigned char chss_cylinder;    // 3        Starting Cylinder
    
    unsigned char type;             // 4        Partition-type Descriptor
                                    // 5 - 7    Ending CHS values
    unsigned char chse_head;        // 5        Ending Head
    unsigned char chse_sector;      // 6        Ending Sector
    unsigned char chse_cylinder;    // 7        Ending Cylinder
    unsigned int lba;               // 8 - 11   Starting Sector
    unsigned int sectors;           // 12 - 15  Partition Size (in sectors)
} __attribute__((packed));


// from https://www.win.tue.nl/~aeb/linux/fs/fat/fat-1.html
// 1.2 Boot sector && FAT32
struct boot_sector_t {                  // Bytes    Content
    unsigned char jmpboot[3];           // 0-2      Jump to bootstrap (E.g. eb 3c 90; on i86: JMP 003E NOP. One finds either eb xx 90, or e9 xx xx. The position of the bootstrap varies.)
    unsigned char oemname[8];           // 3-10     OEM name/version (E.g. "IBM  3.3", "IBM 20.0", "MSDOS5.0", "MSWIN4.0". Various format utilities leave their own name, like "CH-FOR18". Sometimes just garbage. Microsoft recommends "MSWIN4.1".) /* BIOS Parameter Block starts here */
    unsigned short bytes_per_sector;    // 11-12    Number of bytes per sector (512) Must be one of 512, 1024, 2048, 4096.
    unsigned char sector_per_cluster;   // 13       Number of sectors per cluster (1) Must be one of 1, 2, 4, 8, 16, 32, 64, 128. A cluster should have at most 32768 bytes. In rare cases 65536 is OK.
    unsigned short reserved_sector_cnt; // 14-15    Number of reserved sectors (1)
    unsigned char fat_cnt;              // 16       Number of FAT copies (2)
    unsigned short root_entry_cnt;      // 17-18    Number of root directory entries (224)
    unsigned short old_sector_cnt;      // 19-20    Total number of sectors in the filesystem (2880)
    unsigned char media;                // 21       Media descriptor type
    unsigned short sector_per_fat16;    // 22-23    Number of sectors per FAT
    unsigned short sector_per_track;    // 24-25    Number of sectors per track
    unsigned short head_cnt;            // 26-27    Number of heads
    unsigned int hidden_sector_cnt;     // 28-31    Number of hidden sectors Default Boot Sector ends here
    unsigned int sector_cnt;            // 32-35    Total number of sectors in the filesystem  FAT32 definition
    unsigned int sector_per_fat32;      // 36-39    Sectors per FAT
    unsigned short extflags;            // 40-41    Mirror flags
    unsigned short ver;                 // 42-43    Filesystem version
    unsigned int root_cluster;          // 44-47    First cluster of root directory (usually 2)
    unsigned short info;                // 48-49    Filesystem information sector number in FAT32 reserved area (usually 1)
    unsigned short bkbooksec;           // 50-51    Backup boot sector location or 0 or 0xffff if none (usually 6)
    unsigned char reserved[12];         // 52-63    Reserved
    unsigned char drvnum;               // 64       Logical Drive Number (for use with INT 13, e.g. 0 or 0x80)
    unsigned char reserved1;            // 65       Reserved - used to be Current Head (used by Windows NT)
    unsigned char bootsig;              // 66       Extended signature (0x29) Indicates that the three following fields are present.
    unsigned int volid;                 // 67-70    Serial number of partition
    unsigned char vollab[11];           // 71-81    Volume label    
    unsigned char fstype[8];            // 82-89    Filesystem type (E.g. "FAT12   ", "FAT16   ", "FAT     ", "NTFS    ")
} __attribute__((packed));

// from https://www.win.tue.nl/~aeb/linux/fs/fat/fat-1.html
// 1.5 FSInfo sector     File Allocation Table (FAT)
struct fat_info_t {
    struct boot_sector_t bs;  // boot_sector from MBR Partition Table
    unsigned int fat_lba;     // Starting Sector                       (FAT  Region in FAT fs Definition)
    unsigned int cluster_lba; // The number of sector for one fat      (Data Region in FAT32 fs Definition)
};

// attr of dir_t
// https://www.win.tue.nl/~aeb/linux/fs/fat/fat-1.html
// 1.4 Directory Entry
#define ATTR_READ_ONLY  0x01    // 0000 0001  read only
#define ATTR_HIDDEN     0x02    // 0000 0010  hidden
#define ATTR_SYSTEM     0x04    // 0000 0100  system file
#define ATTR_VOLUME_ID  0x08    // 0000 1000  volume id
#define ATTR_LFN        0x0f    // 0000 1111  long file name
#define ATTR_DIRECTORY  0x10    // 0001 0000  directory
#define ATTR_ARCHIVE    0x20    // 0010 0000  archive
#define ATTR_FILE_DIR_MASK (ATTR_DIRECTORY | ATTR_ARCHIVE)

// Directory structure for FAT SFN
// https://www.win.tue.nl/~aeb/linux/fs/fat/fat-1.html
// 1.4 Directory Entry
struct dir_t {                  // Bytes    Content
    unsigned char name[11];     // 0-10     File name (8 bytes) with extension (3 bytes)
    unsigned char attr;         // 11       Attribute - a bitvector.
    unsigned char ntres;        // 12       is reserved for Windows NT
                                // bytes 13-17: creation date and time
    unsigned char crttimetenth; // 13       centiseconds 0-199
    unsigned short crttime;     // 14-15    time as above
    unsigned short crtdate;     // 16-17    date as above
    unsigned short lstaccdate;  // 18-19    date of last access
    unsigned short ch;          // 20-21    are reserved for OS/2
    unsigned short wrttime;     // 22-23    Time (5/6/5 bits, for hour/minutes/doubleseconds)
    unsigned short wrtdate;     // 24-25    Date (7/4/5 bits, for year-since-1980/month/day)
    unsigned short cl;          // 26-27    Starting cluster (0 for an empty file)
    unsigned int size;          // 28-31    Filesize in bytes

} __attribute__((packed));

// Directory structure for FAT LFN
// https://www.win.tue.nl/~aeb/linux/fs/fat/fat-1.html
// 1.4 Directory Entry && VFAT
struct long_dir_t {             // Bytes    Content
    unsigned char order;        // 0        Bits 0-4: sequence number; bit 6: final part of name
    unsigned char name1[10];    // 1-10     Unicode characters 1-5
    unsigned char attr;         // 11       Attribute: 0xf
    unsigned char type;         // 12       Type: 0
    unsigned char checksum;     // 13       Checksum of short name
    unsigned char name2[12];    // 14-25    Unicode characters 6-11
    unsigned short fstcluslo;   // 26-27    Starting cluster: 0
    unsigned char name3[4];     // 28-31    Unicode characters 12-13

} __attribute__((packed));

struct cluster_entry_t {
    union {
        unsigned int val;
        struct {
            unsigned int idx: 28;
            unsigned int reserved: 4;
        };
    };
};

struct filename_t {
    union {
        unsigned char fullname[256];
        struct {
            unsigned char name[13];
        } part[20];
    };
} __attribute__((packed));

struct fat_file_block_t {
    /* Link fat_file_block_t */
    struct list_head list;
    unsigned int oid;     // offset if of file, N = offset N* BLOCK_SIZE of file
    unsigned int cid;     // cluster id
    unsigned int bufIsUpdated;
    unsigned int isDirty;
    unsigned char buf[BLOCK_SIZE];
};

struct fat_file_t {
    /* Head of fat_file_block_t chain */
    struct list_head list;
    unsigned int size;
};

struct fat_dir_t {
    /* Head of fat32_inode chain */
    struct list_head list;
};

struct fat_mount_t {
    /* Link fat_mount_t */
    struct list_head list;
    struct mount *mount;
};

// type of struct fat32_inode
#define FAT_DIR     1
#define FAT_FILE    2

struct fat32_inode {
    const char        *name;
    struct vnode      *node; // redirect to vnode
    struct fat_info_t *fat;
    /* Link fat32_inode */
    struct list_head  list;
    /* cluster id */
    unsigned int      cid;  // cluster id
    unsigned int      type;
    union {
        struct fat_dir_t  *dir;
        struct fat_file_t *file;
    };
};

int register_fat32();

int fat32_mount(struct filesystem *fs, struct mount *mount);
int fat32_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int fat32_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int fat32_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);
int fat32_isdir(struct vnode *dir_node);
int fat32_getname(struct vnode *dir_node, const char **name);
int fat32_getsize(struct vnode *dir_node);

int  fat32_write(struct file *file, const void *buf, size_t len);
int  fat32_read(struct file *file, void *buf, size_t len);
int  fat32_open(struct vnode *file_node, struct file **target);
int  fat32_close(struct file *file);
long fat32_lseek64(struct file *file, long offset, int whence);
int  fat32_sync(struct filesystem *fs);

unsigned int alloc_cluster(struct fat_info_t *fat, unsigned int prev_cid);
unsigned int get_next_cluster(unsigned int fat_lba, unsigned int cluster_id);

struct vnode *_create_vnode(struct vnode *parent, const char *name, unsigned int type, unsigned int cid, unsigned int size);

int            _lookup_cache(struct vnode *dir_node, struct vnode **target, const char *component_name);
struct dir_t* __lookup_fat32(struct vnode *dir_node, const char *component_name, unsigned char *buf, int *buflba);
int            _lookup_fat32(struct vnode *dir_node, struct vnode **target, const char *component_name);

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

#endif
