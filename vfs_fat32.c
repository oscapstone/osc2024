#include "vfs_fat32.h"
#include "sdhost.h"
#include "memory.h"
#include "string.h"
#include "u_list.h"
#include <stdarg.h>
#include "uart1.h"
#include "exception.h"

struct list_head mounts; // Store all new mountpoints that probably need to be sync'd

struct vnode_operations fat32_v_ops = {
    fat32_lookup, fat32_create, fat32_mkdir,
    fat32_isdir, fat32_getname, fat32_getsize,
    fat32_ls, fat32_dump};

struct file_operations fat32_f_ops = {
    fat32_write,
    fat32_read,
    fat32_open,
    fat32_close,
    fat32_lseek64,
};

int register_fat32()
{
    struct filesystem fs;
    fs.name = "fat32";
    fs.setup_mount = fat32_mount;
    fs.sync = fat32_sync;
    return register_filesystem(&fs);
}

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
    partition = (struct partition_t *)&buf[0x1be];
    // check Boot Record Signature, constants
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
    // uart_sendlinek("lba: 0x%x\n", lba);

    readblock(partition[0].lba, buf);

    node = kmalloc(sizeof(struct vnode));
    data = kmalloc(sizeof(struct fat32_inode));
    fat = kmalloc(sizeof(struct fat_info_t));
    dir = kmalloc(sizeof(struct fat_dir_t));
    newmount = kmalloc(sizeof(struct fat_mount_t));

    memcpy((void *)&fat->bs, (void *)buf, sizeof(fat->bs));

    // According to FAT fs design, FAT Region is after reserved sectors
    // https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system

    fat->fat_lba = lba + fat->bs.reserved_sector_cnt;
    fat->cluster_lba = fat->fat_lba + fat->bs.fat_cnt * fat->bs.sector_per_fat32;
    // uart_sendlinek("fat->cluster_lba: 0x%x\n", fat->cluster_lba);

    INIT_LIST_HEAD(&dir->list);

    // oldnode should attach mount the fs
    mount->root = kmalloc(sizeof(struct vnode));
    oldnode = mount->root;
    oldnode->mount = 0;
    oldnode->v_ops = &fat32_v_ops;
    oldnode->f_ops = &fat32_f_ops;
    oldnode->internal = data;

    data->boot_sector = kmalloc(sizeof(struct boot_sector_t));
    memcpy((void *)data->boot_sector, (void *)buf, sizeof(struct boot_sector_t));

    struct boot_sector_t *bs = data->boot_sector;
    int file_block = bs->hidden_sector_cnt + bs->reserved_sector_cnt + bs->fat_cnt * bs->sector_per_fat32;

    uart_sendlinek("bs->hidden_sector_cnt: 0x%x\n", bs->hidden_sector_cnt);
    uart_sendlinek("bs->reserved_sector_cnt: 0x%x\n", bs->reserved_sector_cnt);
    uart_sendlinek("bs->fat_cnt: 0x%x\n", bs->fat_cnt);
    uart_sendlinek("bs->sector_per_fat32: 0x%x\n", bs->sector_per_fat32);
    uart_sendlinek("file_block: 0x%x\n", file_block);

    readblock(fat->cluster_lba, buf);

    // char *n = buf;
    struct sfn_file *file_ptr = buf;

    int high_block;
    int low_block;
    int block;
    unsigned long file_size;
    struct fat32_inode *tmp;
    for (int i = 0; i < 16; i++)
    {
        high_block = file_ptr->first_cluster_high;
        low_block = file_ptr->first_cluster_low;
        block = fat->cluster_lba + high_block * 256 + low_block - 2;
        file_size = file_ptr->file_size;
        if (!file_size)
            break;
        uart_sendlinek("name : %s\n", file_ptr->name);
        uart_sendlinek("high_block: %d\n", high_block);
        uart_sendlinek("low_block: %d\n", low_block);
        uart_sendlinek("file_size: 0x%x\n", file_size);

        data->entry[i] = kmalloc(sizeof(struct vnode));
        data->entry[i]->f_ops = &fat32_f_ops;
        data->entry[i]->v_ops = &fat32_v_ops;
        oldnode->internal = kmalloc(sizeof(struct fat32_inode));
        tmp = (struct fat32_inode *)oldnode->internal;
        memcpy(tmp->Name, file_ptr->name, 11);
        tmp->sfn = file_ptr;
        tmp->datasize = ALIGN_UP(file_size, BLOCK_SIZE);
        tmp->data = kmalloc(tmp->datasize);
        for (int j = 0; j < tmp->datasize / BLOCK_SIZE; j++)
        {
            readblock(block + j, tmp->data + j * BLOCK_SIZE);
        }

        file_ptr++;
    }

    // oldnode = mount->root;
    // uart_sendlinek("mount->root: 0x%x",oldnode);

    // node->mount = oldnode->mount;
    // node->v_ops = oldnode->v_ops;
    // node->f_ops = oldnode->f_ops;
    // node->parent = oldnode->parent;
    // node->internal = oldnode->internal;

    // data->node = node;
    // data->fat = fat;
    // data->cid = 2;        // In FAT32, Root Directory Table starts from cluster #2
    // data->type = FAT_DIR;
    // data->dir = dir;

    // oldnode->mount = mount;
    // oldnode->v_ops = &fat32_v_ops;
    // oldnode->f_ops = &fat32_f_ops;
    // oldnode->internal = data;

    lock();
    list_add(&newmount->list, &mounts);
    unlock();
    newmount->mount = mount;

    return 0;
}

int fat32_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    // if (((struct fat32_inode *)dir_node->internal)->type != FAT_DIR)
    //     return -1;
    // if (!_lookup_cache(dir_node, target, component_name))
    //     return 0;
    // return _lookup_fat32(dir_node, target, component_name);
    struct fat32_inode *dir_inode = dir_node->internal;
    int child_idx = 0;
    for (; child_idx < 16; child_idx++)
    {
        struct vnode *vnode = dir_inode->entry[child_idx];
        if (!vnode)
            break;
        struct fat32_inode *inode = vnode->internal;
        if (strcasecmp(component_name, inode->name) == 0)
        {
            *target = vnode;
            return 0;
        }
    }
    return -1;
}

// ========================================================================================

// struct vnode_operations fat32_v_ops = {
//     fat32_lookup, fat32_create, fat32_mkdir,
//     fat32_isdir, fat32_getname, fat32_getsize,
//     fat32_ls,fat32_dump};

// struct file_operations fat32_f_ops = {
//     fat32_write,
//     fat32_read,
//     fat32_open,
//     fat32_close,
//     fat32_lseek64,
// };

// int register_fat32()
// {
//     struct filesystem fs;
//     fs.name = "fat32";
//     fs.setup_mount = fat32_mount;
//     fs.sync = fat32_sync;
//     INIT_LIST_HEAD(&mounts);
//     return register_filesystem(&fs);
// }

// /* filesystem methods */
// int fat32_mount(struct filesystem *fs, struct mount *mount)
// {
//     struct partition_t *partition;
//     struct fat_info_t *fat;
//     struct fat_dir_t *dir;
//     struct fat32_inode *data;
//     struct vnode *oldnode, *node;
//     struct fat_mount_t *newmount;
//     unsigned int lba;
//     unsigned char buf[BLOCK_SIZE];

//     // Read partition table from MBR, which is at first block
//     readblock(0, buf);
//     // MBR format
//     // 000 ~ 1BD: Code area
//     // 1BE ~ 1FD: Master Partition Table
//     // 1FE ~ 1FF: Boot Record Signature

//     // https://lexra.pixnet.net/blog/post/303910876
//     partition = (struct partition_t *)&buf[0x1be];
//     // check Boot Record Signature, constants
//     if (buf[0x1fe] != 0x55 || buf[0x1ff] != 0xaa)
//     {
//         return -1;
//     }
//     // https://en.wikipedia.org/wiki/Partition_type
//     if (partition[0].type != 0xb && partition[0].type != 0xc) // check Partition type, only FAT32
//     {
//         return -1;
//     }
//     lba = partition[0].lba;
//     readblock(partition[0].lba, buf);

//     node = kmalloc(sizeof(struct vnode));
//     data = kmalloc(sizeof(struct fat32_inode));
//     fat = kmalloc(sizeof(struct fat_info_t));
//     dir = kmalloc(sizeof(struct fat_dir_t));
//     newmount = kmalloc(sizeof(struct fat_mount_t));

//     memcpy((void *)&fat->bs, (void *)buf, sizeof(fat->bs));

//     // According to FAT fs design, FAT Region is after reserved sectors
//     // https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system
//     fat->fat_lba = lba + fat->bs.reserved_sector_cnt;
//     fat->cluster_lba = fat->fat_lba + fat->bs.fat_cnt * fat->bs.sector_per_fat32;

//     INIT_LIST_HEAD(&dir->list);

//     // oldnode should attach mount the fs
//     mount->root = kmalloc(sizeof(struct vnode));
//     oldnode = mount->root;
//     oldnode->mount = 0;
//     oldnode->v_ops = &fat32_v_ops;
//     oldnode->f_ops = &fat32_f_ops;
//     oldnode->internal = data;

//     data->node = oldnode;
//     data->fat = fat;
//     data->cid = 2;        // In FAT32, Root Directory Table starts from cluster #2
//     data->type = FAT_DIR;
//     data->dir = dir;

//     // oldnode = mount->root;
//     // uart_sendlinek("mount->root: 0x%x",oldnode);

//     // node->mount = oldnode->mount;
//     // node->v_ops = oldnode->v_ops;
//     // node->f_ops = oldnode->f_ops;
//     // node->parent = oldnode->parent;
//     // node->internal = oldnode->internal;

//     // data->node = node;
//     // data->fat = fat;
//     // data->cid = 2;        // In FAT32, Root Directory Table starts from cluster #2
//     // data->type = FAT_DIR;
//     // data->dir = dir;

//     // oldnode->mount = mount;
//     // oldnode->v_ops = &fat32_v_ops;
//     // oldnode->f_ops = &fat32_f_ops;
//     // oldnode->internal = data;

//     lock();
//     list_add(&newmount->list, &mounts);
//     unlock();
//     newmount->mount = mount;

//     return 0;
// }

// int fat32_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name)
// {
//     if (((struct fat32_inode *)dir_node->internal)->type != FAT_DIR)
//         return -1;
//     if (!_lookup_cache(dir_node, target, component_name))
//         return 0;
//     return _lookup_fat32(dir_node, target, component_name);
// }

int fat32_create(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    if (((struct fat32_inode *)dir_node->internal)->type != FAT_DIR)
        return -1;
    if (!fat32_lookup(dir_node, target, component_name))
        return -1;
    *target = _create_vnode(dir_node, component_name, FAT_FILE, -1, 0);
    return 0;
}

int fat32_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    if (((struct fat32_inode *)dir_node->internal)->type != FAT_DIR)
        return -1;
    if (!fat32_lookup(dir_node, target, component_name))
        return -1;
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
    if (((struct fat32_inode *)dir_node->internal)->type == FAT_DIR)
        return 0;
    return ((struct fat32_inode *)dir_node->internal)->file->size;
}

/* file_operations methods */
int fat32_write(struct file *file, const void *buf, size_t len)
{
    struct fat32_inode *data;
    int filesize;
    int ret;

    if (fat32_isdir(file->vnode))
        return -1;
    if (!len)
        return len;

    filesize = fat32_getsize(file->vnode);
    data = file->vnode->internal;

    ret = _writefile(buf, data, file->f_pos, len);
    if (ret <= 0)
    {
        return ret;
    }

    file->f_pos += ret;
    if (file->f_pos > filesize)
    {
        data->file->size = file->f_pos;
    }

    return ret;
}

int fat32_read(struct file *file, void *buf, size_t len)
{
    struct fat32_inode *data;
    int filesize;
    int ret;

    if (fat32_isdir(file->vnode))
        return -1;

    filesize = fat32_getsize(file->vnode);
    data = file->vnode->internal;

    if (file->f_pos + len > filesize)
    {
        len = filesize - file->f_pos;
    }
    if (!len)
        return len;

    ret = _readfile(buf, data, file->f_pos, len);
    if (ret <= 0)
        return ret;

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

    if (!fat32_isdir(file->vnode))
        return -1;

    filesize = fat32_getsize(file->vnode);

    if (filesize < 0)
        return -1;

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
    while (fat_lba < fat->cluster_lba)
    {
        found = 0;
        readblock(fat_lba, buf);

        for (int i = 0; i < CLUSTER_ENTRY_PER_BLOCK; ++i)
        {
            ce = &(((struct cluster_entry_t *)buf)[i]);

            if (!ce->val)
            {
                found = 1;
                break;
            }

            ++cid;
        }

        if (found)
        {
            break;
        }
        fat_lba += 1;
    }
    // Only called if prev_cid is larger than 0xfffffff8, realloc ce;
    if (found && prev_cid)
    {
        unsigned int target_lba;
        unsigned int target_idx;

        target_lba = fat_lba + prev_cid / CLUSTER_ENTRY_PER_BLOCK;
        target_idx = prev_cid % CLUSTER_ENTRY_PER_BLOCK;
        readblock(target_lba, buf);
        ce = &(((struct cluster_entry_t *)buf)[target_idx]);
        ce->val = cid;
        writeblock(target_lba, buf);
    }

    if (!found)
    {
        uart_sendlinek("fat32 alloc_cluster: No space!");
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

    if (type == FAT_DIR)
    {
        struct fat_dir_t *dir;
        dir = kmalloc(sizeof(struct fat_dir_t));
        INIT_LIST_HEAD(&dir->list);
        data->dir = dir;
    }
    else
    {
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

    list_for_each_entry(entry, &dir->list, list)
    {
        if (!strcasecmp(component_name, entry->name))
        { // same as strcmp, but tr [:lower:]
            found = 1;
            break;
        }
    }
    if (!found)
    {
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
    int found, dirend, lfn;

    data = dir_node->internal;
    fat = data->fat;
    cid = data->cid;

    found = 0;
    dirend = 0;

    memset(&name, 0, sizeof(struct filename_t));

    while (1)
    {
        int lba;

        lba = fat->cluster_lba + (cid - 2) * fat->bs.sector_per_cluster; // cluster start from #2
        readblock(lba, buf);

        // only called by sync, to update buffer
        if (buflba)
        {
            *buflba = lba;
        }

        for (int i = 0; i < DIR_PER_BLOCK; ++i)
        {
            unsigned char len;
            dir = (struct dir_t *)(&buf[sizeof(struct dir_t) * i]);

            if (dir->name[0] == 0)
            {
                dirend = 1;
                break;
            }

            if ((dir->attr & ATTR_LFN) == ATTR_LFN)
            {
                struct long_dir_t *ldir;
                int n;

                lfn = 1;

                ldir = (struct long_dir_t *)dir;
                n = (dir->name[0] & 0x3f) - 1; // every lfn entry has 16 bytes
                // update filename_t if it is LFN
                for (int i = 0; ldir->name1[i] != 0xff && i < 10; i += 2)
                {
                    name.part[n].name[i / 2] = ldir->name1[i]; // LDIR_Name1 1th ~ 5th
                }
                for (int i = 0; ldir->name2[i] != 0xff && i < 12; i += 2)
                {
                    name.part[n].name[5 + i / 2] = ldir->name2[i]; // LDIR_Name2 6th ~ 11th
                }
                for (int i = 0; ldir->name3[i] != 0xff && i < 4; i += 2)
                {
                    name.part[n].name[11 + i / 2] = ldir->name3[i]; // LDIR_Name3 12th ~ 13th
                }

                continue;
            }

            if (lfn == 1)
            {
                if (!strcasecmp(component_name, (void *)name.fullname))
                {
                    found = 1;
                    break;
                }

                lfn = 0;
                memset(&name, 0, sizeof(struct filename_t));
                continue;
            }

            lfn = 0;
            len = 8;

            // SFN lookup
            while (len)
            {
                if (dir->name[len - 1] != 0x20)
                {
                    break;
                }
                len -= 1;
            }

            memcpy((void *)name.fullname, (void *)dir->name, len);
            name.fullname[len] = 0;

            len = 3;

            while (len)
            {
                if (dir->name[8 + len - 1] != 0x20)
                {
                    break;
                }
                len -= 1;
            }

            if (len >= 0)
            {
                strcat((void *)name.fullname, ".");
                strncat((void *)name.fullname, (void *)&dir->name[8], len);
            }

            if (!strcasecmp(component_name, (void *)name.fullname))
            {
                found = 1;
                break;
            }

            memset(&name, 0, sizeof(struct filename_t));
        }

        if (found)
        {
            break;
        }

        if (dirend)
        {
            break;
        }

        // try next cluster and lookup again
        cid = get_next_cluster(fat->fat_lba, cid);

        if (cid >= INVALID_CID)
        {
            break;
        }
    }

    if (!found)
    {
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

    if (!dir)
    {
        return -1;
    }

    if (!(dir->attr & ATTR_FILE_DIR_MASK))
    {
        return -1;
    }

    cid = (dir->ch << 16) | dir->cl; // upper part of cluster id + lower part of cluster id

    if (dir->attr & ATTR_ARCHIVE)
    {
        type = FAT_FILE;
    }
    else
    {
        type = FAT_DIR;
    }

    node = _create_vnode(dir_node, component_name, type, cid, dir->size);

    *target = node;

    return 0;
}

int _writefile_seek_cache(struct fat32_inode *data, unsigned int foid, struct fat_file_block_t **block)
{
    struct fat_file_block_t *entry;
    struct list_head *head;

    head = &data->file->list;

    if (list_empty(head))
    {
        return -1;
    }

    list_for_each_entry(entry, head, list)
    {
        *block = entry;
        if (foid == entry->oid)
        { // offset match
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
    if (*block)
    {
        curoid = (*block)->oid;
        curcid = (*block)->cid;

        if (curoid == foid)
        { // offset match
            return 0;
        }

        curoid++;
        curcid = get_next_cluster(info->fat_lba, curcid);
    }
    else
    {
        curoid = 0;
        curcid = fcid;
    }

    // create new block in cache
    while (1)
    {
        struct fat_file_block_t *newblock;

        newblock = kmalloc(sizeof(struct fat_file_block_t));

        newblock->oid = curoid;
        newblock->cid = curcid;
        newblock->bufIsUpdated = 0;
        newblock->isDirty = 1;

        list_add_tail(&newblock->list, &data->file->list);

        *block = newblock;

        if (curoid == foid)
        {
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
    if (!block->bufIsUpdated)
    {
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
    if (cid >= INVALID_CID)
    {
        memset(block->buf, 0, BLOCK_SIZE);
    }
    else
    {
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

int _writefile(const void *buf, struct fat32_inode *data, unsigned long long fileoff, unsigned long long len)
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
    ret = _writefile_seek_cache(data, foid, &block);

    if (ret < 0)
    {
        ret = _writefile_seek_fat32(data, foid, cid, &block);
    }

    if (ret < 0)
    {
        return 0;
    }

    // Write
    while (len)
    {
        unsigned long long bckoff;

        bckoff = (fileoff + result) % BLOCK_SIZE;

        if (&block->list != head)
        {
            // cache has the blocks to be written
            ret = _writefile_cache(data, bckoff, buf, bufoff, len, block);
            cid = block->cid;
            block = list_first_entry(&block->list, struct fat_file_block_t, list);
        }
        else
        {
            // Read block from sdcard, create cache, then write it
            cid = get_next_cluster(data->fat->fat_lba, cid);
            ret = _writefile_fat32(data, bckoff, buf, bufoff, len, coid, cid);
        }

        if (ret < 0)
        {
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
    if (list_empty(head))
    {
        return -1;
    }

    // find that offset in cache
    list_for_each_entry(entry, head, list)
    {
        *block = entry;
        if (foid == entry->oid)
        {
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
    if (*block)
    {
        curoid = (*block)->oid;
        curcid = (*block)->cid;

        if (curoid == foid)
        {
            return 0;
        }

        curoid++;
        curcid = get_next_cluster(info->fat_lba, curcid);

        if (curcid >= INVALID_CID)
        {
            return -1;
        }
    }
    else
    {
        curoid = 0;
        curcid = fcid;
    }

    while (1)
    {
        // add new offset block to cache
        struct fat_file_block_t *newblock;

        newblock = kmalloc(sizeof(struct fat_file_block_t));

        newblock->oid = curoid;
        newblock->cid = curcid;
        newblock->bufIsUpdated = 0;
        newblock->isDirty = 0;

        list_add_tail(&newblock->list, &data->file->list);

        *block = newblock;

        if (curoid == foid)
        {
            return 0;
        }

        curoid++;
        curcid = get_next_cluster(info->fat_lba, curcid);

        if (curcid >= INVALID_CID)
        {
            return -1;
        }
    }
}

int _readfile_cache(struct fat32_inode *data, unsigned long long bckoff, unsigned char *buf, unsigned long long bufoff, unsigned int size, struct fat_file_block_t *block)
{
    int rsize;

    if (!block->bufIsUpdated)
    {
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
    if (ret < 0)
    {
        ret = _readfile_seek_fat32(data, foid, cid, &block);
    }

    if (ret < 0)
    {
        return 0;
    }

    // Read
    while (len)
    {
        unsigned long long bckoff;

        bckoff = (fileoff + result) % BLOCK_SIZE;

        if (&block->list != head)
        {
            // cache found
            ret = _readfile_cache(data, bckoff, buf, bufoff, len, block);

            cid = block->cid;
            block = list_first_entry(&block->list, struct fat_file_block_t, list);
        }
        else
        {
            // Read block from sdcard, create cache
            cid = get_next_cluster(data->fat->fat_lba, cid);

            if (cid >= INVALID_CID)
            {
                break;
            }

            ret = _readfile_fat32(data, bckoff, buf, bufoff, len, coid, cid);
        }

        if (ret < 0)
        {
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
    struct long_dir_t *ldir;
    unsigned int cid;
    int lba, idx, lfnidx;
    unsigned char buf[BLOCK_SIZE];

    data = dirnode->internal;
    head = &data->dir->list;
    cid = data->cid;
    idx = 0;
    lfnidx = 1;

    if (cid >= INVALID_CID)
    {
        uart_sendlinek("fat32 _do_sync_dir: invalid dirnode->data->cid");
    }

    // get the buf from dirnode's offset
    lba = data->fat->cluster_lba + (cid - 2) * data->fat->bs.sector_per_cluster;
    readblock(lba, buf);

    list_for_each_entry(entry, head, list)
    {
        struct dir_t *origindir;
        const char *name;
        const char *ext;
        int lfn, namelen, extpos, i, buflba;
        unsigned char lookupbuf[BLOCK_SIZE];

        name = entry->name;

        // If entry is a old file, update its size
        origindir = __lookup_fat32(dirnode, name, lookupbuf, &buflba);

        if (origindir)
        {
            if (entry->type == FAT_FILE)
            {
                origindir->size = entry->file->size;
                writeblock(buflba, lookupbuf);
            }
            continue;
        }

        // Else if entry is a new file
        ext = NULL;
        extpos = -1;

        do
        {

            namelen = strlen(name);
            if (namelen >= 13)
            {
                lfn = 1;
                break;
            }
            for (i = 0; i < namelen; ++i)
            {
                // get idx of extension name
                if (name[namelen - 1 - i] == '.')
                {
                    break;
                }
            }
            if (i < namelen)
            {
                ext = &name[namelen - i];
                extpos = namelen - 1 - i;
            }
            if (i >= 4)
            {
                lfn = 1;
                break;
            }
            if (namelen - 1 - i > 8)
            {
                // SFN: 8.3
                lfn = 1;
                break;
            }

            lfn = 0;
        } while (0);

        // Seek idx to the end of dir
        while (1)
        {
            dir = (struct dir_t *)(&buf[sizeof(struct dir_t) * idx]);

            if (dir->name[0] == 0)
            {
                break;
            }

            idx += 1;

            if (idx >= 16)
            { // if idx is over Directory Entry, create a new one
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

        // Write LFN
        if (lfn)
        {
            int ord;
            int first;

            ord = ((namelen - 1) / 13) + 1;
            first = 0x40; // LAST_LONG_ENTRY flag in LFN LDIR_Ord

            for (; ord > 0; --ord)
            {
                int end;
                ldir = (struct long_dir_t *)(&buf[sizeof(struct long_dir_t) * idx]);

                ldir->order = first | ord;
                ldir->attr = ATTR_LFN;
                ldir->type = 0;
                // TODO: Calculate checksum, SFN + LFN
                ldir->checksum = 0;
                ldir->fstcluslo = 0;

                first = 0;
                end = 0;

                for (i = 0; i < 10; i += 2)
                {
                    if (end)
                    { // padding 0xff if filename end
                        ldir->name1[i] = 0xff;
                        ldir->name1[i + 1] = 0xff;
                    }
                    else
                    {
                        ldir->name1[i] = name[(ord - 1) * 13 + i / 2];
                        ldir->name1[i + 1] = 0;
                        if (ldir->name1[i] == 0)
                        {
                            end = 1;
                        }
                    }
                }
                for (i = 0; i < 12; i += 2)
                {
                    if (end)
                    {
                        ldir->name2[i] = 0xff;
                        ldir->name2[i + 1] = 0xff;
                    }
                    else
                    {
                        ldir->name2[i] = name[(ord - 1) * 13 + 5 + i / 2];
                        ldir->name2[i + 1] = 0;
                        if (ldir->name2[i] == 0)
                        {
                            end = 1;
                        }
                    }
                }
                for (i = 0; i < 4; i += 2)
                {
                    if (end)
                    {
                        ldir->name3[i] = 0xff;
                        ldir->name3[i + 1] = 0xff;
                    }
                    else
                    {
                        ldir->name3[i] = name[(ord - 1) * 13 + 11 + i / 2];
                        ldir->name3[i + 1] = 0;
                        if (ldir->name3[i] == 0)
                        {
                            end = 1;
                        }
                    }
                }

                idx += 1;

                if (idx >= 16)
                {
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

        if (entry->type == FAT_DIR)
        {
            dir->attr = ATTR_DIRECTORY;
            dir->size = 0;
        }
        else
        {
            dir->attr = ATTR_ARCHIVE;
            dir->size = entry->file->size;
        }

        if (entry->cid >= INVALID_CID)
        {
            entry->cid = alloc_cluster(data->fat, 0);
        }

        dir->ch = (entry->cid >> 16) & 0xffff;
        dir->cl = entry->cid & 0xffff;

        if (lfn)
        {
            int lfni;

            // Creating SFN body
            // TODO: handle lfnidx
            for (i = 7, lfni = lfnidx; i >= 0 && lfni;)
            {
                dir->name[i--] = '0' + lfni % 10;
                lfni /= 10;
            }

            lfnidx++;
            // numeric-tail: ~n (1 <= n <= 6)
            dir->name[i--] = '~';

            // TODO: handle letter case
            memcpy((void *)dir->name, name, i + 1);
        }
        else
        {
            // TODO: handle letter case
            for (i = 0; i != extpos && name[i]; ++i)
            {
                dir->name[i] = name[i];
            }

            for (; i < 8; ++i)
            {
                dir->name[i] = ' '; // in SFN, each part is padded with space
            }
        }

        // SFN format: 8.3
        // TODO: handle letter case
        for (i = 0; i < 3 && ext[i]; ++i)
        {
            dir->name[8 + i] = ext[i];
        }

        for (; i < 3; ++i)
        {
            dir->name[8 + i] = ' ';
        }

        idx += 1;

        if (idx >= 16)
        {
            int newcid;

            writeblock(lba, buf);

            newcid = get_next_cluster(data->fat->fat_lba, cid);
            if (newcid >= INVALID_CID)
            {
                newcid = alloc_cluster(data->fat, cid);
            }

            cid = newcid;

            lba = data->fat->cluster_lba + (cid - 2) * data->fat->bs.sector_per_cluster;

            // TODO: Cache data block of directory
            readblock(lba, buf);

            idx = 0;
        }
    }

    if (idx)
    {
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

    if (cid >= INVALID_CID)
    {
        // uart_sendlinek("fat32 _do_sync_file: invalid cid");
        return;
    }

    list_for_each_entry(entry, head, list)
    {
        int lba;

        if (entry->oid == 0)
        {
            // cache is valid but not sync'd, just an informational warning
            if (!(entry->cid >= INVALID_CID) && data->cid != entry->cid)
            {
                uart_sendlinek("_do_sync_file: cid isn't sync");
            }

            entry->cid = data->cid;
        }

        if (entry->cid >= INVALID_CID)
        {
            entry->cid = alloc_cluster(data->fat, cid);
        }

        // if the file is sync'd, pass
        if (!entry->isDirty)
        {
            continue;
        }

        // cache buf is not updated, initialize it to avoid error
        if (!entry->bufIsUpdated)
        {
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

    list_for_each_entry(entry, head, list)
    {
        if (entry->type == FAT_DIR)
        {
            _sync_dir(entry->node); // recursive to next dirnode
        }
        else
        {
            _do_sync_file(entry->node);
        }
    }
}

int fat32_op_deny()
{
    return -1;
}

void fat32_dump(struct vnode *vnode, int level)
{
}

void fat32_ls(struct vnode *vnode)
{
    uart_sendlinek("in fat32_ls\n");
    struct fat32_inode *inode = (struct fat32_inode *)vnode->internal;
    int child_idx = 0;
    for (; child_idx <= 16; child_idx++)
    {
        if (!inode->entry[child_idx])
        {
            // break;
        }
        vnode = inode->entry[child_idx];
        uart_sendlinek("%s\n", ((struct fat32_inode *)vnode->internal)->name);
    }
}