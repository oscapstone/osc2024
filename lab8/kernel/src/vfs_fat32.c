#include "vfs_fat32.h"
#include "sdhost.h"
#include "memory.h"
#include "string.h"
#include "u_list.h"
#include "debug.h"
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
    fat32_getsize};

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
    unsigned char filedata_buf[BLOCK_SIZE];

    // Read partition table from MBR, which is at first block
    readblock(0, buf);
    struct MBR *mbr = kmalloc(sizeof(struct MBR));
    memcpy(mbr, buf, sizeof(struct MBR));
    // MBR format
    // 000 ~ 1BD: Code area
    // 1BE ~ 1FD: Master Partition Table
    // 1FE ~ 1FF: Boot Record Signature

    // https://lexra.pixnet.net/blog/post/303910876
    partition = &(mbr->partition_table[0]);
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

    // int high_block;
    // int low_block;
    int block;
    unsigned long file_size;
    struct fat32_inode *tmp;
    char name_array[16];
    for (int i = 0; i < 16; i++)
    {
        // high_block = file_ptr->first_cluster_high;
        // low_block = file_ptr->first_cluster_low;
        // block = file_block + high_block * 256 + low_block - 2;
        block = file_block + get_first_cluster(file_ptr) - 2;
        file_size = file_ptr->file_size;
        if (file_ptr->name[0] == (char *)0)
            break;
        uart_sendlinek("name : %s\n", file_ptr->name);
        // uart_sendlinek("high_block: %d\n", high_block);
        // uart_sendlinek("low_block: %d\n", low_block);
        uart_sendlinek("file_size: 0x%x\n", file_size);
        uart_sendlinek("fat32_f_ops addr: 0x%x\n", &fat32_f_ops);

        data->entry[i] = kmalloc(sizeof(struct vnode));
        data->entry[i]->f_ops = &fat32_f_ops;
        data->entry[i]->v_ops = &fat32_v_ops;
        data->entry[i]->internal = kmalloc(sizeof(struct fat32_inode));
        tmp = (struct fat32_inode *)(data->entry[i]->internal);
        fat32_get_file_name(tmp->name, file_ptr);
        uart_sendlinek("name_array : %s\n", tmp->name);
        tmp->sfn = file_ptr;
        tmp->datasize = file_size;

        // tmp->data = kmalloc(tmp->datasize);
        // for (int j = 0; j < tmp->datasize / BLOCK_SIZE; j++)
        // {
        //     readblock(block + j, tmp->data + j * BLOCK_SIZE);
        // }
        tmp->data_block_cnt = block;
        // tmp->data = kmalloc(BLOCK_SIZE);
        // readblock(block, filedata_buf);
        // uart_sendlinek("%s\n", filedata_buf);
        // memcpy(tmp->data, filedata_buf, BLOCK_SIZE);

        file_ptr++;
    }

    for (int i = 0; i < 16; i++)
    {
        if (data->entry[i] != 0)
        {
            uart_sendlinek("%d\n", i);
            uart_sendlinek("%s\n", ((struct fat32_inode *)data->entry[i]->internal)->name);
        }
    }

    int max_cluster_num = (bs->sector_per_fat32 * bs->bytes_per_sector / sizeof(unsigned int));
    uart_sendlinek("sector_per_fat32 : %d\n", bs->sector_per_fat32);
    uart_sendlinek("bytes_per_sector : %d\n", bs->bytes_per_sector);
    uart_sendlinek("max_cluster_num : %d\n", max_cluster_num);
    file_allocation_table = kmalloc(max_cluster_num);

    for (int i = 0; i < bs->sector_per_fat32; i++)
        readblock(bs->hidden_sector_cnt + bs->reserved_sector_cnt + i, file_allocation_table + i * bs->bytes_per_sector / sizeof(unsigned int));

    readblock(bs->hidden_sector_cnt + bs->info, buf);
    fs_info = kmalloc(sizeof(struct fs_info));
    memcpy(fs_info, buf, sizeof(struct fs_info));
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
        // uart_sendlinek("vnode name : %s\n", inode->Name);
        // uart_sendlinek("vnode name size: %d\n", strlen(inode->Name));
        // uart_sendlinek("component_name %s\n", component_name);
        // uart_sendlinek("component_name size: %d\n", strlen(component_name));
        if (strcmp(component_name, inode->name) == 0)
        {
            // uart_sendlinek("GET %s\n", component_name);
            *target = vnode;
            return 0;
        }
    }
    return -1;
}

int fat32_create(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    uart_sendlinek("component_name : %s\n", component_name);

    unsigned char buf[BLOCK_SIZE];
    unsigned char name[FAT32_MAX_FILENAME] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
    unsigned char extension[FAT32_MAX_EXTENSION] = {0x20, 0x20, 0x20};
    struct fat32_inode *inode = dir_node->internal;
    struct boot_sector_t *bs = inode->boot_sector;

    int file_block = bs->hidden_sector_cnt + bs->reserved_sector_cnt + bs->fat_cnt * bs->sector_per_fat32;
    readblock(file_block, buf);
    struct sfn_file *file_ptr = buf;
    for (int i = 0; i < 16; i++, file_ptr++)
    {
        uart_sendlinek("%s\n", file_ptr->name);
        if (file_ptr->name[0] == (char *)0)
        {
            uart_sendlinek("%d\n", i);
            break;
        }
    }
    int name_cnt = 0;
    for (; name_cnt < strlen(component_name); name_cnt++)
    {
        if (component_name[name_cnt] == '.' || component_name[name_cnt] == '\0')
        {
            break;
        }
        if (name_cnt < FAT32_MAX_FILENAME)
        {
            name[name_cnt] = component_name[name_cnt];
        }
    }
    if (component_name[name_cnt] == '.')
    {
        name_cnt++;
        for (int i = 0; name_cnt < strlen(component_name); i++, name_cnt++)
        {
            if (component_name[name_cnt] == '\0')
            {
                break;
            }
            if (i < FAT32_MAX_EXTENSION)
            {
                extension[i] = component_name[name_cnt];
            }
        }
    }
    // uart_sendlinek("new file name : %s\n", name);
    // uart_sendlinek("new file extension : %s\n", extension);
    memcpy(file_ptr->name, name, FAT32_MAX_FILENAME);
    memcpy(file_ptr->extension, extension, FAT32_MAX_EXTENSION);
    // ===============================================================================================

    int max_cluster_num = (bs->sector_per_fat32 * bs->bytes_per_sector / sizeof(unsigned int));
    // find_free_cluster
    int free_cluster = fs_info->next_free;
    for (; free_cluster < max_cluster_num; free_cluster++)
    {
        if (file_allocation_table[free_cluster] == FAT32_FREE_CLUSTER)
        {
            break;
        }
    }
    uart_sendlinek("free_cluster : %d\n", free_cluster);
    file_ptr->attribute = 0x20;
    file_ptr->creation_time_tenth_seconds = 0;
    file_ptr->creation_time = 0;
    file_ptr->creation_date = 0;
    file_ptr->last_access_date = 0;
    file_ptr->first_cluster_high = (free_cluster & 0xFFFF0000) >> 16;
    file_ptr->last_write_time = 0;
    file_ptr->last_write_date = 0;
    file_ptr->first_cluster_low = free_cluster & 0x0000FFFF;
    file_ptr->file_size = 512;
    file_allocation_table[free_cluster] = FAT32_END_OF_CHAIN;

    writeblock(file_block, buf);

    struct vnode *_vnode = kmalloc(sizeof(struct vnode));

    _vnode->f_ops = &fat32_f_ops;
    _vnode->v_ops = &fat32_v_ops;
    _vnode->internal = kmalloc(sizeof(struct fat32_inode));
    struct fat32_inode *_inode = (struct fat32_inode *)(_vnode->internal);
    fat32_get_file_name(_inode->name, file_ptr);
    _inode->sfn = file_ptr;
    _inode->datasize = 512;
    *target = _vnode;

    int find_entry = 0;
    for (; find_entry < 16; find_entry++)
    {
        if (!inode->entry[find_entry])
        {
            uart_sendlinek("%d\n", find_entry);
            break;
        }
    }
    inode->entry[find_entry] = _vnode;

    return 0;
};

int fat32_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    return 0;
};
int fat32_isdir(struct vnode *dir_node)
{
    return 0;
};
int fat32_getname(struct vnode *dir_node, const char **name)
{
    return 0;
};
int fat32_getsize(struct vnode *dir_node)
{
    return 0;
};
int fat32_write(struct file *file, const void *buf, size_t len)
{
    unsigned char filedata_buf[BLOCK_SIZE];
    // char *pt = buf;
    struct fat32_inode *inode = file->vnode->internal;
    // memcpy(filedata_buf,buf,BLOCK_SIZE);
    // uart_sendlinek("IN fat32_write\n");
    // uart_sendlinek("old file->f_pos : %d\n",file->f_pos);
    uart_sendlinek("fat32_write : %s\n", buf);
    readblock(inode->data_block_cnt, filedata_buf);
    // uart_sendlinek("old filedata_buf : %s\n",filedata_buf);
    // char *tmp = &filedata_buf[file->f_pos];
    memcpy(filedata_buf + file->f_pos, buf, len);
    uart_sendlinek("filedata_buf : %s\n", filedata_buf);
    writeblock(inode->data_block_cnt, filedata_buf);

    inode->datasize = len + file->f_pos > inode->datasize ? len + file->f_pos : inode->datasize;
    // uart_sendlinek("new file->f_pos : %d\n",file->f_pos);
    // uart_sendlinek("new filedata_buf : %s\n",filedata_buf);
    // uart_sendlinek("new filedata_buf size: %d\n",sizeof(filedata_buf));
    return len;
};
int fat32_read(struct file *file, void *buf, size_t len)
{
    // uart_sendlinek("in fat32_read\n");
    unsigned char filedata_buf[BLOCK_SIZE];

    struct fat32_inode *inode = file->vnode->internal;
    readblock(inode->data_block_cnt, filedata_buf);
    len = len > inode->datasize ? inode->datasize : len;
    memcpy(buf, filedata_buf, len);
    return len;
};
int fat32_open(struct vnode *file_node, struct file **target)
{
    // uart_sendlinek("fat32_f_ops addr: 0x%x\n", file_node->f_ops);
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
long fat32_lseek64(struct file *file, long offset, int whence)
{
    return 0;
};
int fat32_sync(struct filesystem *fs)
{
    return 0;
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
    char name_array[16];
    struct fat32_inode *inode = (struct fat32_inode *)vnode->internal;
    int child_idx = 0;
    for (; child_idx <= 16; child_idx++)
    {
        if (!inode->entry[child_idx])
        {
            break;
        }
        vnode = inode->entry[child_idx];
        // fat32_get_file_name(name_array,((struct fat32_inode *)vnode->internal)->sfn);
        // uart_sendlinek("%s\n", name_array);
        uart_sendlinek("%s\n", ((struct fat32_inode *)vnode->internal)->name);
    }
}
