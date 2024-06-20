#include "fat32.h"
#include "memory.h"
#include "sdhost.h"
#include "string.h"
#include "list.h"
#include <stdarg.h>
#include "uart1.h"
#include "exception.h"

struct list_head mounts; // Store all new mountpoints that probably need to be sync'd

struct vnode_operations fat32_v_ops = {
    fat32_lookup, fat32_create, fat32_mkdir,
    fat32_isdir, fat32_getname, fat32_getsize
};

struct file_operations fat32_f_ops = {
    fat32_write, fat32_read, fat32_open,
    fat32_close, fat32_lseek64,
};

int register_fat32()
{
    struct filesystem fs;
    fs.name = "fat32";
    fs.setup_mount = fat32_mount;
    fs.sync = fat32_sync;
    INIT_LIST_HEAD(&mounts);
    return register_filesystem(&fs);
}

/* filesystem methods */
int fat32_mount(struct filesystem *fs, struct mount *mount)
{
    struct partition_t *partition;
    struct fat_info_t *fat;
    struct fat_dir_t *dir;
    struct fat32_inode *data;
    struct vnode *oldnode, *node;
    struct fat_mount_t *newmount;
    unsigned int lba;
    unsigned char buf[BLOCK_SIZE];

    // Read partition table from MBR, which is at first block
    readblock(0, buf);
    // MBR format
    // 000 ~ 1BD: Code area
    // 1BE ~ 1FD: Master Partition Table
    // 1FE ~ 1FF: Boot Record Signature

    // https://lexra.pixnet.net/blog/post/303910876
    // from https://thestarman.pcministry.com/asm/mbr/PartTables.htm
    // The Master Boot Record (MBR) Table 1
    partition = (struct partition_t *)&buf[0x1be];


    // check Boot Record Signature, constants
    // from https://thestarman.pcministry.com/asm/mbr/PartTables.htm
    // The Master Boot Record (MBR) Table 2
    if (buf[0x1fe] != 0x55 || buf[0x1ff] != 0xaa)
    {
        return -1;
    }
    // https://en.wikipedia.org/wiki/Partition_type
    if (partition[0].type != 0xb && partition[0].type != 0xc) // check Partition type, only FAT32
    {
        return -1;
    }
    lba = partition[0].lba;

    // read the sector from SD card
    readblock(partition[0].lba, buf);

    node        = kmalloc(sizeof(struct vnode));
    data        = kmalloc(sizeof(struct fat32_inode));
    fat         = kmalloc(sizeof(struct fat_info_t));
    dir         = kmalloc(sizeof(struct fat_dir_t));
    newmount    = kmalloc(sizeof(struct fat_mount_t));

    // copy the boot sector to fat32_inode
    memcpy((void *)&fat->bs, (void *)buf, sizeof(fat->bs));

    // According to FAT fs design, FAT Region is after reserved sectors
    // https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system

    //  fat Starting Sector =   Starting Sector   +   Number of reserved sectors
    fat->fat_lba            =   lba               +   fat->bs.reserved_sector_cnt;   

    // fat num of Sector    =   Starting Sector   +   Number of FAT copies    *   Sectors per FAT
    fat->cluster_lba        =   fat->fat_lba      +   fat->bs.fat_cnt         *   fat->bs.sector_per_fat32;

    INIT_LIST_HEAD(&dir->list);

    // oldnode should attach mount the fs
    // oldnode = mount->root;
    mount->root = kmalloc(sizeof(struct vnode));
    oldnode = mount->root;

    // make the vnode
    // node->mount = oldnode->mount;       // is 0
    // node->v_ops = oldnode->v_ops;
    // node->f_ops = oldnode->f_ops;
    // node->parent = oldnode->parent;
    // node->internal = oldnode->internal;

    // make the inode
    data->node = oldnode;
    data->fat = fat;
    data->cid = 2;        // In FAT32, Root Directory Table starts from cluster #2
    data->type = FAT_DIR;
    data->dir = dir;

    oldnode->mount = mount;
    oldnode->v_ops = &fat32_v_ops;
    oldnode->f_ops = &fat32_f_ops;
    oldnode->internal = data;

    lock();
    list_add(&newmount->list, &mounts);
    unlock();
    newmount->mount = mount;

    return 0;
}

int fat32_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    if (((struct fat32_inode *)dir_node->internal)->type != FAT_DIR) return -1;
    if (!_lookup_cache(dir_node, target, component_name)) return 0;
    return _lookup_fat32(dir_node, target, component_name);
}

int fat32_create(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    if (((struct fat32_inode *)dir_node->internal)->type != FAT_DIR) return -1;
    if (!fat32_lookup(dir_node, target, component_name)) return -1;
    *target = _create_vnode(dir_node, component_name, FAT_FILE, -1, 0);
    return 0;
}

int fat32_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    if (((struct fat32_inode *)dir_node->internal)->type != FAT_DIR) return -1;
    if (!fat32_lookup(dir_node, target, component_name)) return -1;
    *target = _create_vnode(dir_node, component_name, FAT_DIR, -1, 0);
    return 0;
}

int fat32_isdir(struct vnode *dir_node)
{
    return (((struct fat32_inode *)dir_node->internal)->type == FAT_DIR) ? 1 : 0;
}

int fat32_getname(struct vnode *dir_node, const char **name)
{
    *name = ((struct fat32_inode *)dir_node->internal)->name;
    return 0;
}

int fat32_getsize(struct vnode *dir_node)
{
    if (((struct fat32_inode *)dir_node->internal)->type == FAT_DIR) return 0;
    return ((struct fat32_inode *)dir_node->internal)->file->size;
}

/* file_operations methods */
int fat32_write(struct file *file, const void *buf, size_t len)
{
    struct fat32_inode *data;
    int filesize;
    int ret;

    if (fat32_isdir(file->vnode)) return -1;
    if (!len) return len;

    filesize = fat32_getsize(file->vnode);
    data = file->vnode->internal;

    ret = _writefile(buf, data, file->f_pos, len);
    if (ret <= 0) {
        return ret;
    }

    file->f_pos += ret;
    if (file->f_pos > filesize) {
        data->file->size = file->f_pos;
    }

    return ret;
}

int fat32_read(struct file *file, void *buf, size_t len)
{
    struct fat32_inode *data;
    int filesize;
    int ret;

    if (fat32_isdir(file->vnode)) return -1;

    filesize = fat32_getsize(file->vnode);
    data = file->vnode->internal;

    if (file->f_pos + len > filesize) {
        len = filesize - file->f_pos;
    }
    if (!len) return len;

    ret = _readfile(buf, data, file->f_pos, len);
    if (ret <= 0) return ret;

    file->f_pos += ret;
    return ret;
}

int fat32_open(struct vnode *file_node, struct file **target)
{
    (*target)->vnode = file_node;
    (*target)->f_pos = 0;
    (*target)->f_ops = file_node->f_ops;
    return 0;
}

int fat32_close(struct file *file)
{
    file->vnode = NULL;
    file->f_pos = 0;
    file->f_ops = NULL;
    return 0;
}

long fat32_lseek64(struct file *file, long offset, int whence)
{
    int filesize;
    int base;

    if (!fat32_isdir(file->vnode)) return -1;

    filesize = fat32_getsize(file->vnode);

    if (filesize < 0) return -1;

    switch (whence) 
    {
        case SEEK_SET:
            base = 0;
            break;
        case SEEK_CUR:
            base = file->f_pos;
            break;
        case SEEK_END:
            base = filesize;
            break;
        default:
            return -1;
    }

    if (base + offset > filesize) 
    {
        return -1;
    }
    file->f_pos = base + offset;

    return 0;
}

int fat32_sync(struct filesystem *fs)
{
    struct fat_mount_t *entry;

    list_for_each_entry(entry, &mounts, list) 
    {
        _sync_dir(entry->mount->root);
    }

    return 0;
}

// ------------------------------------------------------------------------
// Utils
//

/*
    The get_next_cluster function retrieves the next cluster ID in a chain for a given cluster_id in a FAT file system. It calculates the block and index within the FAT where the cluster entry is located, reads the block into a buffer, and returns the value of the specific cluster entry.
    @fat_lba: The Logical Block Address (LBA) of the start of the File Allocation Table (FAT).
    @cluster_id: The current cluster ID for which we want to find the next cluster in the chain.
*/
unsigned int get_next_cluster(unsigned int fat_lba, unsigned int cluster_id)
{
    struct cluster_entry_t *ce;
    unsigned int idx;
    unsigned char buf[BLOCK_SIZE];

    if (cluster_id >= INVALID_CID) 
    {
        return cluster_id;
    }

    // Get fat allocation table and its cluster entry in Data region
    fat_lba += cluster_id / CLUSTER_ENTRY_PER_BLOCK;
    idx = cluster_id % CLUSTER_ENTRY_PER_BLOCK;

    readblock(fat_lba, buf);
    ce = &(((struct cluster_entry_t *)buf)[idx]);

    return ce->val;
}

unsigned int alloc_cluster(struct fat_info_t *fat, unsigned int prev_cid)
{
    struct cluster_entry_t *ce;
    unsigned int fat_lba;
    unsigned int cid;
    int found;
    unsigned char buf[BLOCK_SIZE];

    fat_lba = fat->fat_lba;
    cid = 0;
    // find unused cluster by clusters -> fats
    while (fat_lba < fat->cluster_lba) {
        found = 0;
        readblock(fat_lba, buf);

        for (int i = 0; i < CLUSTER_ENTRY_PER_BLOCK; ++i) {
            ce = &(((struct cluster_entry_t *)buf)[i]);

            if (!ce->val) {
                found = 1;
                break;
            }

            ++cid;
        }

        if (found) {
            break;
        }
        fat_lba += 1;
    }

    // Only called if prev_cid is larger than 0xfffffff8, realloc ce;
    if (found && prev_cid) {
        unsigned int target_lba;
        unsigned int target_idx;
        
        target_lba = fat_lba + prev_cid / CLUSTER_ENTRY_PER_BLOCK;
        target_idx = prev_cid % CLUSTER_ENTRY_PER_BLOCK;
        readblock(target_lba, buf);
        ce = &(((struct cluster_entry_t *)buf)[target_idx]);
        ce->val = cid;
        writeblock(target_lba, buf);
    }

    if (!found) {
        uart_sendline("fat32 alloc_cluster: No space!");
        return -1;
    }

    return cid;
}
/* vnode_operations methods */
struct vnode *_create_vnode(struct vnode *parent, const char *name, unsigned int type, unsigned int cid, unsigned int size)
{
    struct vnode *node;
    struct fat32_inode *info, *data;
    char *buf;
    int len;

    info = parent->internal;
    len = strlen(name);

    buf = kmalloc(len + 1);
    node = kmalloc(sizeof(struct vnode));
    data = kmalloc(sizeof(struct fat32_inode));

    strcpy(buf, name);

    data->name = buf;
    data->node = node;
    data->fat = info->fat;
    data->cid = cid;
    data->type = type;

    if (type == FAT_DIR) {
        struct fat_dir_t *dir;
        dir = kmalloc(sizeof(struct fat_dir_t));
        INIT_LIST_HEAD(&dir->list);
        data->dir = dir;
    } else {
        struct fat_file_t *file;
        file = kmalloc(sizeof(struct fat_file_t));
        INIT_LIST_HEAD(&file->list);
        file->size = size;
        data->file = file;
    }

    node->mount = parent->mount;
    node->v_ops = &fat32_v_ops;
    node->f_ops = &fat32_f_ops;
    node->parent = parent;
    node->internal = data;

    // attach to parent's dir_t or file_t list
    list_add(&data->list, &info->dir->list);

    return node;
}

int _lookup_cache(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct fat32_inode *data, *entry;
    struct fat_dir_t *dir;
    int found;

    data = dir_node->internal;
    dir = data->dir;
    found = 0;

    list_for_each_entry(entry, &dir->list, list) {
        if (!strcasecmp(component_name, entry->name)) { // same as strcmp, but tr [:lower:]
            found = 1;
            break;
        }
    }
    if (!found) {
        return -1;
    }
    *target = entry->node;
    return 0;
}

struct dir_t *__lookup_fat32(struct vnode *dir_node, const char *component_name, unsigned char *buf, int *buflba)
{
    struct dir_t *dir;
    struct fat32_inode *data;
    struct fat_info_t *fat;
    struct filename_t name;
    unsigned int cid;
    int found, dirend;
    // int lfn;

    data = dir_node->internal;
    fat = data->fat;
    cid = data->cid;

    found = 0;
    dirend = 0;

    memset(&name, 0, sizeof(struct filename_t));

    // lookup cluster by cluster
    while (1) {
        int lba;

        lba = fat->cluster_lba + (cid - 2) * fat->bs.sector_per_cluster; // cluster start from #2
        readblock(lba, buf);

        // only called by sync, to update buffer
        if (buflba) {
            *buflba = lba;
        }

        for (int i = 0; i < DIR_PER_BLOCK; ++i) {
            unsigned char len;
            dir = (struct dir_t *)(&buf[sizeof(struct dir_t) * i]);
            
            if (dir->name[0] == 0) {
                dirend = 1;
                break;
            }

            len = 8;

            // SFN lookup
            while (len) {
                if (dir->name[len - 1] != 0x20) { // 0x20 is space
                    break;
                }
                len -= 1;
            }

            memcpy((void *)name.fullname, (void *)dir->name, len);
            name.fullname[len] = 0;

            len = 3;

            while (len) {
                if (dir->name[8 + len - 1] != 0x20) { // 0x20 is space
                    break;
                }
                len -= 1;
            }

            if (len >= 0) {
                strcat((void *)name.fullname, ".");
                strncat((void *)name.fullname, (void *)&dir->name[8], len);
            }

            if (!strcasecmp(component_name, (void *)name.fullname)) {
                found = 1;
                break;
            }

            memset(&name, 0, sizeof(struct filename_t));
        }

        if (found) {
            break;
        }

        if (dirend) {
            break;
        }

        // try next cluster and lookup again
        cid = get_next_cluster(fat->fat_lba, cid);

        if (cid >= INVALID_CID) {
            break;
        }
    }

    if (!found) {
        return NULL;
    }

    return dir;
}

int _lookup_fat32(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct vnode *node;
    struct dir_t *dir;
    unsigned int type, cid;
    unsigned char buf[BLOCK_SIZE];

    dir = __lookup_fat32(dir_node, component_name, buf, NULL);

    if (!dir) {
        return -1;
    }

    if (!(dir->attr & ATTR_FILE_DIR_MASK)) {
        return -1;
    }

    cid = (dir->ch << 16) | dir->cl; // upper part of cluster id + lower part of cluster id

    if (dir->attr & ATTR_ARCHIVE) {
        type = FAT_FILE;
    } else {
        type = FAT_DIR;
    }

    node = _create_vnode(dir_node, component_name, type, cid, dir->size);

    *target = node;

    return 0;
}


/*

  
*/
int _writefile_seek_cache(struct fat32_inode *data, unsigned int foid, struct fat_file_block_t **block)
{
    struct fat_file_block_t *entry;
    struct list_head *head;

    head = &data->file->list;

    if (list_empty(head)) {
        return -1;
    }

    list_for_each_entry(entry, head, list) {
        *block = entry;
        if (foid == entry->oid) { // offset match
            return 0;
        }
    }

    return -1;
}

int _writefile_seek_fat32(struct fat32_inode *data, unsigned int foid, unsigned int fcid, struct fat_file_block_t **block)
{
    struct fat_info_t *info;
    unsigned int curoid, curcid;

    info = data->fat;

    // block is from cache
    if (*block) {
        curoid = (*block)->oid;
        curcid = (*block)->cid;

        if (curoid == foid) { // offset match
            return 0;
        }

        curoid++;
        curcid = get_next_cluster(info->fat_lba, curcid);
    } else {
        curoid = 0;
        curcid = fcid;
    }

    // create new block in cache
    while (1) {
        struct fat_file_block_t *newblock;

        newblock = kmalloc(sizeof(struct fat_file_block_t));

        newblock->oid = curoid;
        newblock->cid = curcid;
        newblock->bufIsUpdated = 0;
        newblock->isDirty = 1;

        list_add_tail(&newblock->list, &data->file->list);

        *block = newblock;

        if (curoid == foid) {
            return 0;
        }

        curoid++;
        curcid = get_next_cluster(info->fat_lba, curcid);
    }
}

int _writefile_cache(struct fat32_inode *data, unsigned long long bckoff, const unsigned char *buf, unsigned long long bufoff, unsigned int size, struct fat_file_block_t *block)
{
    int wsize;

    // if cache block is not updated yet, read from sdcard
    if (!block->bufIsUpdated) {
        // read the data from sdcard
        struct fat_info_t *info;
        int lba;

        info = data->fat;
        lba = info->cluster_lba + (block->cid - 2) * info->bs.sector_per_cluster;

        readblock(lba, block->buf);

        block->bufIsUpdated = 1;
    }

    wsize = size > BLOCK_SIZE - bckoff ? BLOCK_SIZE - bckoff : size;
    memcpy((void *)&block->buf[bckoff], (void *)&buf[bufoff], wsize);

    return wsize;
}

int _writefile_fat32(struct fat32_inode *data, unsigned long long bckoff, const unsigned char *buf, unsigned int bufoff, unsigned int size, unsigned int oid, unsigned int cid)
{
    struct list_head *head;
    struct fat_info_t *info;
    struct fat_file_block_t *block;
    int lba;
    int wsize;

    head = &data->file->list;
    info = data->fat;

    block = kmalloc(sizeof(struct fat_file_block_t));

    wsize = size > BLOCK_SIZE - bckoff ? BLOCK_SIZE - bckoff : size;
    // read the address from sdcard and write it
    if (cid >= INVALID_CID) {
        memset(block->buf, 0, BLOCK_SIZE);
    } else {
        lba = info->cluster_lba + (cid - 2) * info->bs.sector_per_cluster;

        readblock(lba, block->buf);
    }

    memcpy((void *)&block->buf[bckoff], (void *)&buf[bufoff], wsize);

    block->oid = oid;
    block->cid = cid;
    block->bufIsUpdated = 1;
    block->isDirty = 1; // wait for sync

    list_add_tail(&block->list, head);

    return wsize;
}

/*
    The _writefile function writes data to a file in a FAT file system. It first seeks the cache for the block to be written, then seeks the FAT32 file system for the block to be written. It then writes the data to the cache or the FAT32 file system, depending on the availability of the block in the cache.
    @buf: The buffer containing the data to be written.
    @data: The FAT32 inode structure representing the file to which the data is to be written.
    @fileoff: The offset in the file at which the data is to be written.
    @len: The length of the data to be written.
*/

int _writefile(const void *buf, struct fat32_inode *data, unsigned long long fileoff, unsigned long long len)
{
    struct fat_file_block_t *block;
    struct list_head *head;
    unsigned int foid;      // first block id
    unsigned int coid;      // current block id
    unsigned int cid;       // target cluster id
    unsigned long long bufoff, result;
    int ret;

    block = NULL;
    head = &data->file->list;
    foid = fileoff / BLOCK_SIZE;
    coid = 0;
    cid = data->cid;
    bufoff = 0;
    result = 0;

    // Seek, find the particular offset
    ret = _writefile_seek_cache(data, foid, &block);

    if (ret < 0) {
        ret = _writefile_seek_fat32(data, foid, cid, &block);
    }

    if (ret < 0) {
        return 0;
    }

    // Write
    while (len) {
        unsigned long long bckoff;

        bckoff = (fileoff + result) % BLOCK_SIZE;

        if (&block->list != head) {
            // cache has the blocks to be written
            ret = _writefile_cache(data, bckoff, buf, bufoff, len, block);
            cid = block->cid;
            block = list_first_entry(&block->list, struct fat_file_block_t, list);
        } else {
            // Read block from sdcard, create cache, then write it
            cid = get_next_cluster(data->fat->fat_lba, cid);
            ret = _writefile_fat32(data, bckoff, buf, bufoff, len, coid, cid);
        }

        if (ret < 0) {
            break;
        }

        bufoff += ret;
        result += ret;
        coid += 1;
        len -= ret;
    }

    return result;
}


int _readfile_seek_cache(struct fat32_inode *data, unsigned int foid, struct fat_file_block_t **block)
{
    struct fat_file_block_t *entry;
    struct list_head *head;

    // check the cache list
    head = &data->file->list;

    // if cache is none, return not found
    if (list_empty(head)) {
        return -1;
    }

    // find that offset in cache
    list_for_each_entry(entry, head, list) {
        *block = entry;
        if (foid == entry->oid) {
            return 0;
        }
    }

    return -1;
}

int _readfile_seek_fat32(struct fat32_inode *data, unsigned int foid, unsigned int fcid, struct fat_file_block_t **block)
{
    struct fat_info_t *info;
    unsigned int curoid, curcid;

    info = data->fat;

    // if block found in cache, change to that offset in fat32
    if (*block) {
        curoid = (*block)->oid;
        curcid = (*block)->cid;

        if (curoid == foid) {
            return 0;
        }

        curoid++;
        curcid = get_next_cluster(info->fat_lba, curcid);

        if (curcid >= INVALID_CID) {
            return -1;
        }
    } else {
        curoid = 0;
        curcid = fcid;
    }

    while (1) {
        // add new offset block to cache
        struct fat_file_block_t *newblock;

        newblock = kmalloc(sizeof(struct fat_file_block_t));

        newblock->oid = curoid;
        newblock->cid = curcid;
        newblock->bufIsUpdated = 0;
        newblock->isDirty = 0;

        list_add_tail(&newblock->list, &data->file->list);

        *block = newblock;

        if (curoid == foid) {
            return 0;
        }

        curoid++;
        curcid = get_next_cluster(info->fat_lba, curcid);

        if (curcid >= INVALID_CID) {
            return -1;
        }
    }
}

int _readfile_cache(struct fat32_inode *data, unsigned long long bckoff, unsigned char *buf, unsigned long long bufoff, unsigned int size, struct fat_file_block_t *block)
{
    int rsize;

    if (!block->bufIsUpdated) {
        // read the data from sdcard
        struct fat_info_t *info;
        int lba;

        info = data->fat;
        lba = info->cluster_lba + (block->cid - 2) * info->bs.sector_per_cluster;
        readblock(lba, block->buf);
        
        block->bufIsUpdated = 1;
    }

    // if the size is overflow, resize it
    rsize = size > BLOCK_SIZE - bckoff ? BLOCK_SIZE - bckoff : size;

    memcpy((void *)&buf[bufoff], (void *)&block->buf[bckoff], rsize);

    return rsize;
}

int _readfile_fat32(struct fat32_inode *data, unsigned long long bckoff, unsigned char *buf, unsigned int bufoff, unsigned int size, unsigned int oid, unsigned int cid)
{
    struct list_head *head;
    struct fat_info_t *info;
    struct fat_file_block_t *block;
    int lba;
    int rsize;

    head = &data->file->list;
    info = data->fat;

    block = kmalloc(sizeof(struct fat_file_block_t));

    rsize = size > BLOCK_SIZE - bckoff ? BLOCK_SIZE - bckoff : size;
    lba = info->cluster_lba + (cid - 2) * info->bs.sector_per_cluster;

    // read from fat32
    readblock(lba, block->buf);

    memcpy((void *)&buf[bufoff], (void *)&block->buf[bckoff], rsize);

    block->oid = oid;
    block->cid = cid;
    block->bufIsUpdated = 1;

    list_add_tail(&block->list, head);

    return rsize;
}

int _readfile(void *buf, struct fat32_inode *data, unsigned long long fileoff, unsigned long long len)
{
    struct fat_file_block_t *block;
    struct list_head *head;
    unsigned int foid; // first block id
    unsigned int coid; // current block id
    unsigned int cid;  // target cluster id
    unsigned long long bufoff, result;
    int ret;

    block = NULL;
    head = &data->file->list;
    foid = fileoff / BLOCK_SIZE;
    coid = 0;
    cid = data->cid;
    bufoff = 0;
    result = 0;

    // Seek, find the particular offset
    ret = _readfile_seek_cache(data, foid, &block);
    if (ret < 0) {
        ret = _readfile_seek_fat32(data, foid, cid, &block);
    }

    if (ret < 0) {
        return 0;
    }

    // Read
    while (len) {
        unsigned long long bckoff;

        bckoff = (fileoff + result) % BLOCK_SIZE;

        if (&block->list != head) {
            // cache found
            ret = _readfile_cache(data, bckoff, buf, bufoff, len, block);

            cid = block->cid;
            block = list_first_entry(&block->list, struct fat_file_block_t, list);
        } else {
            // Read block from sdcard, create cache
            cid = get_next_cluster(data->fat->fat_lba, cid);

            if (cid >= INVALID_CID) {
                break;
            }

            ret = _readfile_fat32(data, bckoff, buf, bufoff, len, coid, cid);
        }

        if (ret < 0) {
            break;
        }

        bufoff += ret;
        result += ret;
        coid += 1;
        len -= ret;
    }

    return result;
}


void _do_sync_dir(struct vnode *dirnode)
{
    struct fat32_inode *data, *entry;
    struct list_head *head;
    struct dir_t *dir;
    unsigned int cid;
    int lba, idx;
    unsigned char buf[BLOCK_SIZE];

    data = dirnode->internal;
    head = &data->dir->list;
    cid = data->cid;


    if (cid >= INVALID_CID) {
        uart_sendline("fat32 _do_sync_dir: invalid dirnode->data->cid");
    }

    // get the buf from dirnode's offset
    lba = data->fat->cluster_lba + (cid - 2) * data->fat->bs.sector_per_cluster;
    readblock(lba, buf);

    list_for_each_entry(entry, head, list) {
        struct dir_t *origindir;
        const char *name;
        const char *ext;
        int lfn, namelen, extpos, i, buflba;
        unsigned char lookupbuf[BLOCK_SIZE];

        name = entry->name;

        // If entry is a old file, update its size
        origindir = __lookup_fat32(dirnode, name, lookupbuf, &buflba);

        if (origindir) {
            if (entry->type == FAT_FILE) {
                origindir->size = entry->file->size;
                writeblock(buflba, lookupbuf);
            }
            continue;
        }

        // Else if entry is a new file
        ext = NULL;
        extpos = -1;

        // handle SFN's string
        do {
            namelen = strlen(name);
            if (namelen >= 13) {
                lfn = 1;
                break;
            }
            for (i = 0; i < namelen; ++i) {
                // get idx of extension name
                if (name[namelen - 1 - i] == '.') {
                    break;
                }
            }
            if (i < namelen) {
                ext = &name[namelen - i];
                extpos = namelen - 1 - i;
            }
            if (i >= 4) {
                lfn = 1;
                break;
            }
            if (namelen - 1 - i > 8) {
                // SFN: 8.3
                lfn = 1;
                break;
            }

            lfn = 0;
        } while(0);

        // Seek idx to the end of dir
        while (1) {
            dir = (struct dir_t *)(&buf[sizeof(struct dir_t) * idx]);

            // find the empty dir
            if (dir->name[0] == 0) {
                break;
            }

            idx += 1;

            if (idx >= 16) { // if idx is over Directory Entry, create a new one
                unsigned int newcid;

                writeblock(lba, buf);

                newcid = get_next_cluster(data->fat->fat_lba, cid);
                if (newcid >= INVALID_CID)
                {
                    newcid = alloc_cluster(data->fat, cid);
                }

                cid = newcid;

                lba = data->fat->cluster_lba + (cid - 2) * data->fat->bs.sector_per_cluster;

                readblock(lba, buf);
                idx = 0;
            }
        }

        // Write SFN
        dir = (struct dir_t *)(&buf[sizeof(struct dir_t) * idx]);

        // TODO: Set these properties properly
        dir->ntres = 0;
        dir->crttimetenth = 0;
        dir->crttime = 0;
        dir->crtdate = 0;
        dir->lstaccdate = 0;
        dir->wrttime = 0;
        dir->wrtdate = 0;

        if (entry->type == FAT_DIR) {
            dir->attr = ATTR_DIRECTORY;
            dir->size = 0;
        } else {
            dir->attr = ATTR_ARCHIVE;
            dir->size = entry->file->size;
        }

        if (entry->cid >= INVALID_CID) {
            entry->cid = alloc_cluster(data->fat, 0);
        }

        dir->ch = (entry->cid >> 16) & 0xffff;
        dir->cl = entry->cid & 0xffff;

        // TODO: handle letter case
        for (i = 0; i != extpos && name[i]; ++i)
        {
            dir->name[i] = name[i];
        }

        for (; i < 8; ++i)
        {
            dir->name[i] = ' '; // in SFN, each part is padded with space
        }

        // SFN format: 8.3
        // TODO: handle letter case
        for (i = 0; i < 3 && ext[i]; ++i) {
            dir->name[8 + i] = ext[i];
        }

        for (; i < 3; ++i) {
            dir->name[8 + i] = ' ';
        }

        idx += 1;

        // if the cluster is full, create a new cluster
        if (idx >= 16) {
            int newcid;

            writeblock(lba, buf);

            newcid = get_next_cluster(data->fat->fat_lba, cid);
            if (newcid >= INVALID_CID) {
                newcid = alloc_cluster(data->fat, cid);
            }

            cid = newcid;

            lba = data->fat->cluster_lba + (cid - 2) * data->fat->bs.sector_per_cluster;

            // TODO: Cache data block of directory
            readblock(lba, buf);

            idx = 0;
        }
    }

    if (idx) {
        writeblock(lba, buf);
    }
}

void _do_sync_file(struct vnode *filenode)
{
    struct fat_file_block_t *entry;
    struct fat32_inode *data;
    struct list_head *head;
    unsigned int cid;

    data = filenode->internal;
    head = &data->file->list;
    cid = data->cid;

    if (cid >= INVALID_CID) {
        //uart_sendline("fat32 _do_sync_file: invalid cid");
        return;
    }

    list_for_each_entry(entry, head, list) {
        int lba;

        if (entry->oid == 0) {
            // cache is valid but not sync'd, just an informational warning
            if (!(entry->cid >= INVALID_CID) && data->cid != entry->cid) {
                uart_sendline("_do_sync_file: cid isn't sync");
            }

            entry->cid = data->cid;
        }

        if (entry->cid >= INVALID_CID) {
            entry->cid = alloc_cluster(data->fat, cid);
        }

        // if the file is sync'd, pass
        if (!entry->isDirty) {
            continue;
        }

        // cache buf is not updated, initialize it to avoid error
        if (!entry->bufIsUpdated) {
            memset(entry->buf, 0, BLOCK_SIZE);
            entry->bufIsUpdated = 1;
        }

        // sync to fat32 fs
        lba = data->fat->cluster_lba + (entry->cid - 2) * data->fat->bs.sector_per_cluster;
        writeblock(lba, entry->buf);

        entry->isDirty = 0;

        cid = entry->cid;
    }
}

void _sync_dir(struct vnode *dirnode)
{
    struct fat32_inode *data, *entry;
    struct list_head *head;

    // Init: dirnode is mounted fs
    data = dirnode->internal;
    head = &data->dir->list;

    _do_sync_dir(dirnode);

    list_for_each_entry(entry, head, list) {
        if (entry->type == FAT_DIR) {
            _sync_dir(entry->node);     // recursive to next dirnode
        } else {
            _do_sync_file(entry->node);
        }
    }
}
