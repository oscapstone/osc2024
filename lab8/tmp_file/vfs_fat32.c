#include "vfs_fat32.h"
#include "sdhost.h"
#include "memory.h"
#include "string.h"
#include "u_list.h"
#include <stdarg.h>
#include "uart1.h"
#include "exception.h"

struct list_head mounts; // Store all new mountpoints that probably need to be sync'd

static inline unsigned int get_first_cluster(struct sfn_file *entry)
{
    return (entry->first_cluster_high << 16) | entry->first_cluster_low;
}

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
    struct fat32_inode *data;
    struct vnode *node;
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
    readblock(partition[0].lba, buf);

    data = kmalloc(sizeof(struct fat32_inode));
    data->boot_sector = kmalloc(sizeof(struct boot_sector_t));
    memcpy((void *)data->boot_sector, (void *)buf, sizeof(struct boot_sector_t));

    // oldnode should attach mount the fs
    mount->root = kmalloc(sizeof(struct vnode));
    node = mount->root;
    node->mount = 0;
    node->v_ops = &fat32_v_ops;
    node->f_ops = &fat32_f_ops;
    node->internal = data;

    struct boot_sector_t *bs = data->boot_sector;
    // int file_block = lba + bs->hidden_sector_cnt + bs->reserved_sector_cnt + bs->fat_cnt * bs->sector_per_fat32;
    int file_block = bs->hidden_sector_cnt + bs->reserved_sector_cnt + bs->fat_cnt * bs->sector_per_fat32;

    uart_sendlinek("bs->hidden_sector_cnt: 0x%x\n", bs->hidden_sector_cnt);
    uart_sendlinek("bs->reserved_sector_cnt: 0x%x\n", bs->reserved_sector_cnt);
    uart_sendlinek("bs->fat_cnt: 0x%x\n", bs->fat_cnt);
    uart_sendlinek("bs->sector_per_fat32: 0x%x\n", bs->sector_per_fat32);
    uart_sendlinek("file_block: 0x%x\n", file_block);

    readblock(file_block, buf);

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
        // block = file_block + high_block * 256 + low_block - 2;
        block = get_first_cluster(file_ptr) + file_block + 2;
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
        data->entry[i]->internal = kmalloc(sizeof(struct fat32_inode));
        tmp = (struct fat32_inode *)(data->entry[i]->internal);
        memcpy(tmp->Name, file_ptr->name, 16);
        tmp->sfn = file_ptr;
        tmp->datasize = ALIGN_UP(file_size, BLOCK_SIZE);
        tmp->data = kmalloc(tmp->datasize);
        for (int j = 0; j < tmp->datasize / BLOCK_SIZE; j++)
        {
            readblock(block + j, tmp->data + j * BLOCK_SIZE);
        }
        uart_sendlinek("%s\n", *((char *)tmp->data));
        file_ptr++;
    }

    for (int i = 0; i < 16; i++)
    {
        if (data->entry[i] != 0)
        {
            uart_sendlinek("%d\n", i);
            uart_sendlinek("%s\n", ((struct fat32_inode *)data->entry[i]->internal)->Name);
        }
    }

    return 0;
}

int fat32_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
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

int fat32_create(struct vnode *dir_node, struct vnode **target, const char *component_name) {

};
int fat32_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name) {

};
int fat32_isdir(struct vnode *dir_node) {

};
int fat32_getname(struct vnode *dir_node, const char **name) {

};
int fat32_getsize(struct vnode *dir_node) {

};
int fat32_write(struct file *file, const void *buf, size_t len) {

};
int fat32_read(struct file *file, void *buf, size_t len)
{
    struct fat32_inode *inode = file->vnode->internal;
    // overflow, shrink size
    if (len + file->f_pos > inode->datasize)
    {
        memcpy(buf, inode->data + file->f_pos, inode->datasize - file->f_pos);
        file->f_pos += inode->datasize - file->f_pos;
        return inode->datasize - file->f_pos;
    }
    else
    {
        memcpy(buf, inode->data + file->f_pos, len);
        file->f_pos += len;
        return len;
    }
    return -1;
};
int fat32_open(struct vnode *file_node, struct file **target)
{
    (*target)->vnode = file_node;
    (*target)->f_ops = file_node->f_ops;
    (*target)->f_pos = 0;
    return 0;
};
int fat32_close(struct file *file)
{
    kfree(file);
    return 0;
};
long fat32_lseek64(struct file *file, long offset, int whence) {

};
int fat32_sync(struct filesystem *fs) {

};

int fat32_op_deny()
{
    return -1;
}

void fat32_dump(struct vnode *vnode, int level)
{
}

void fat32_ls(struct vnode *vnode)
{
    // uart_sendlinek("in fat32_ls\n");
    struct fat32_inode *inode = (struct fat32_inode *)vnode->internal;
    int child_idx = 0;
    for (; child_idx <= 16; child_idx++)
    {
        if (!inode->entry[child_idx])
        {
            break;
        }
        vnode = inode->entry[child_idx];
        uart_sendlinek("%s\n", ((struct fat32_inode *)vnode->internal)->Name);
        // uart_sendlinek("0x%x\n", ((struct fat32_inode *)vnode->internal)->datasize);
    }
}