#include "fat32.h"
#include "uart.h"
#include "mm.h"
#include "string.h"
#include "sd.h"
#include "mbr.h"
#include "kernel.h"

#define VFS_DEBUG

struct fat_boot_sector fat_boot_sector;
struct fat32_info fat_info;

int fat32_setup_mount(struct filesystem* fs, struct mount* mount);

int fat32_write(struct file *file, const void *buf, size_t len);
int fat32_read(struct file *file, void *buf, size_t len);
int fat32_open(struct vnode *file_node, struct file **target);
int fat32_close(struct file *file);
long fat32_lseek64(struct file *file, long offset, int whence);

int fat32_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int fat32_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int fat32_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);

void setup_fat32(struct fat32_inode *dir_inode);

unsigned int get_free_fat_cluster(void);
unsigned int read_fat_entry(unsigned int cluster);
void write_fat_table(unsigned int cluster, unsigned int value);
void fat32_new_dir_entry(unsigned int dir_cluster, struct sfn_entry *new_entry);
void print_fat32_dir(unsigned int cluster);

struct filesystem fat32_filesystem = {
    .name = "fat32",
    .setup_mount = fat32_setup_mount,
};

struct file_operations fat32_file_operations = {
    .write   = fat32_write,
    .read    = fat32_read,
    .open    = fat32_open,
    .close   = fat32_close,
    .lseek64 = fat32_lseek64,
};

struct vnode_operations fat32_vnode_operations = {
    .lookup  = fat32_lookup,
    .create  = fat32_create,
    .mkdir   = fat32_mkdir,
};

int fat32_setup_mount(struct filesystem* fs, struct mount* mount)
{
    struct fat32_inode *root_inode;
    char buf[BLOCK_SIZE];

#ifdef VFS_DEBUG
    printf("            [fat32_setup_mount]\n");
#endif

    mount->fs = fs;
    mount->root = fat32_create_vnode(0, FSNODE_TYPE_DIR); // create a root vnode.
    root_inode = mount->root->internal;

    /* Get boot sector and MBR information. */
    get_fat32_boot_sector();
    /* setup an inode for root directory. */
    root_inode->cluster = fat_info.data_region; // the root directory is the first cluster of the data region.
    /* Get the root directory and store at root_inode->data. */
    readblock(fat_info.data_region, buf);
    memcpy(root_inode->data, buf, BLOCK_SIZE);

    root_inode->cluster = fat_boot_sector.root_dir;
    root_inode->abs_block_num = fat_info.data_region + (root_inode->cluster - 2) * fat_info.blocks_per_cluster;
    print_fat32_dir(root_inode->cluster);
    /* Setup all the vnodes and inodes under FAT32 file system. */
    setup_fat32(root_inode);

    return 1;
}

/**
 * Create a fat32 vnode along with the inode below it.
 * We should specify the type of the vnode and the mount structure of it.
 * The name of inode is not setup.
 */
struct vnode *fat32_create_vnode(struct mount *mount, enum fsnode_type type)
{
    struct vnode *node;
    struct fat32_inode *inode;

    /* Create vnode */
    node = (struct vnode*)kmalloc(sizeof(struct vnode));
    node->v_ops = &fat32_vnode_operations;
    node->f_ops = &fat32_file_operations;
    node->mount = mount; // All the vnode should not point back to mount.

    /* Create fat32 inode */
    inode = (struct fat32_inode *)kmalloc(sizeof(struct fat32_inode));
    memset(inode, 0, sizeof(struct fat32_inode));
    inode->type = type;
    inode->data = (char *)kmalloc(BLOCK_SIZE);
    memset(inode->data, 0, BLOCK_SIZE);

    /* Assign inode to vnode */
    node->internal = inode;
    return node;
}

/* file write operation */
int fat32_write(struct file *file, const void *buf, size_t len)
{
    struct fat32_inode *inode;

    if (!len)
        return 0;
    /* Take the file inode. */
    inode = file->vnode->internal;
    /* Copy data from buf to f_pos */
    memcpy(inode->data + file->f_pos, buf, len);
    /* Update data block on SD card. */
    writeblock(inode->abs_block_num, inode->data);
    /* Update f_pos and size */
    file->f_pos += len;

    /* Check boundary. */
    if (inode->data_size < file->f_pos)
        inode->data_size = file->f_pos;

#ifdef VFS_DEBUG
    printf("        [fat32_write] file name: %s, size: %d\n", inode->name, inode->data_size);
#endif
    return len;
}

/* File read operation */
int fat32_read(struct file *file, void *buf, size_t len)
{
    struct fat32_inode *inode;

#ifdef VFS_DEBUG
    printf("        [fat32_read] file name: %s, size: %d\n", ((struct fat32_inode *)file->vnode->internal)->name, len);
#endif

    inode = file->vnode->internal;
    /* Update contents from SD card */
    readblock(inode->abs_block_num, inode->data);
    /*if buffer overflow, shrink the request read length. Then read from f_pos */
    if ((file->f_pos + len) > inode->data_size) {
        len = inode->data_size - file->f_pos;
        memcpy(buf, inode->data + file->f_pos, len);
        file->f_pos += inode->data_size - file->f_pos;
    } else {
        memcpy(buf, inode->data + file->f_pos, len);
        file->f_pos += len;
    }
    return len;
}

/* Newly created struct file should be generated by file_vnode. */
int fat32_open(struct vnode *file_node, struct file **target)
{
    (*target)->vnode = file_node;
    (*target)->f_ops = file_node->f_ops;
    (*target)->f_pos = 0;
    return 1;
}

/* file close opeation. Just free the memory */
int fat32_close(struct file *file)
{
    kfree(file);
    return 1;
}

/* Move the f_pos of the struct file. */
long fat32_lseek64(struct file *file, long offset, int whence)
{
    if (whence == SEEK_SET) {
        file->f_pos = offset;
        return file->f_pos;
    }
    return 0;
}

/**
 * fat32 vnode operations, lookup all the vnodes under dir_node.
 * Returns 0 if error, returns 1 if success.
 */
int fat32_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct fat32_inode *dir_inode, *inode;
    struct vnode *vnode;
    int child_idx;

    /* Search the child inode. */
    dir_inode = dir_node->internal;
    for (child_idx = 0; child_idx < MAX_DIR_NUM; child_idx++) {
        vnode = dir_inode->childs[child_idx];
        if (!vnode)
            break;
        inode = vnode->internal;
        if (!strcmp(component_name, inode->name)) {
            *target = vnode;
            return 1;
        }
    }
#ifdef VFS_DEBUG
    printf("        [fat32_lookup] %s not found\n", component_name);
#endif
    return 0;
}

/* fat32 vnode operation: create a vnode under dir_node, put it to target */
int fat32_create(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct fat32_inode *inode, *child_inode, *new_inode;
    int child_idx, j, idx = 0;
    struct vnode *new_vnode;
    unsigned int new_cluster;
    struct sfn_entry new_entry;

#ifdef VFS_DEBUG
    printf("        [fat32_create] component name: %s\n", component_name);
#endif

    /* Get the inode of dir_node*/
    inode = dir_node->internal;
    if (inode->type != FSNODE_TYPE_DIR) {
        printf("        [fat32_create] not under directory vnode\n");
        return 0;
    }

    for (child_idx = 0; child_idx < MAX_DIR_NUM; child_idx++) {
        if (!inode->childs[child_idx])
            break;
        /* Get the child inode and compare the name. */
        child_inode = inode->childs[child_idx]->internal;
        if (!strcmp(child_inode->name, component_name)) {
            printf("        [fat32_create] file %s already exists\n", component_name);
            return 0;
        }
    }

    if (child_idx >= MAX_DIR_NUM) {
        printf("        [fat32_create] directory entry full.\n");
        return 0;
    }

    if (strlen(component_name) > MAX_FILE_NAME_LEN) {
        printf("        [fat32_create] file name too long.\n");
        return 0;
    }

    /* Create a new vnode and assign it to founded index. */
    new_vnode = fat32_create_vnode(0, FSNODE_TYPE_FILE);
    inode->childs[child_idx] = new_vnode;

    /* Copy the component_name to inode. */
    new_inode = new_vnode->internal;
    strcpy(new_inode->name, component_name);

    /* Create new file on SD card, get an empty cluster number */
    new_cluster = get_free_fat_cluster();
    new_inode->cluster = new_cluster;
    new_inode->abs_block_num = fat_info.data_region + ((new_cluster - 2) * fat_info.blocks_per_cluster);
    /* New a sfn_entry. Put file name to SFN entry. */
    for (j = 0; j < 8; j++) {
        if (component_name[idx] == '.' || component_name[idx] == '\0')
            new_entry.name[j] = ' ';
        else
            new_entry.name[j] = component_name[idx++];
    }
    for (j = 0; j < 3; j++) {
        if (component_name[idx] == '\0')
            new_entry.file_ext[j] = ' ';
        else
            new_entry.file_ext[j] = component_name[++idx];
    }
    new_entry.attr = ATTR_ARCHIVE;
    new_entry.cluster_high = new_cluster >> 16;
    new_entry.cluster_low = new_cluster & 0xffff;
    new_entry.file_size = BLOCK_SIZE;
    /* Update directory entry to SD card */
    fat32_new_dir_entry(inode->cluster, &new_entry);

    /* Update the FAT table */
    write_fat_table(new_cluster, FAT_ENTRY_EOC); // Set the cluster value to 0x0fffffff

    *target = new_vnode;
    return 1;
}

/* vnode operation: create a directory vnode under dir_node, put it to target */
int fat32_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct fat32_inode *inode, *new_inode;
    int child_idx;
    struct vnode* new_vnode;

    inode = dir_node->internal;
    if (inode->type != FSNODE_TYPE_DIR) {
        printf("        [fat32_mkdir] dir_node is not directory\n");
        return -1;
    }

    /* Find the available child_idx. */
    for (child_idx = 0; child_idx < MAX_DIR_NUM; child_idx++)
        if (!inode->childs[child_idx])
            break;
    if (child_idx >= MAX_DIR_NUM) {
        printf("        [fat32_mkdir] directory entry full\n");
        return -1;
    }

    if (strlen(component_name) > MAX_PATH_LEN) {
        printf("        [fat32_mkdir] file name too long\n");
        return -1;
    }

    /* Create a new vnode and put it to inode->childs. */
    new_vnode = fat32_create_vnode(0, FSNODE_TYPE_DIR);
    inode->childs[child_idx] = new_vnode;

    /* Assign component name to the inode->name. */
    new_inode = new_vnode->internal;
    strcpy(new_inode->name, component_name);

    *target = new_vnode;
    return 0;
}

/* Find an empty entry in FAT table, return its cluster number. */
unsigned int get_free_fat_cluster(void)
{
    unsigned int *fat_table, block_idx, i;
    char buf[BLOCK_SIZE];

    for (block_idx = fat_info.fat_region; block_idx < fat_info.fat_region + fat_info.sectors_per_fat; block_idx++) {
        readblock(block_idx, buf);
        fat_table = (unsigned int *)buf;
        for (i = 0; i < BLOCK_SIZE / sizeof(unsigned int); i++)
            if (FAT_ENTRY(fat_table[i]) == FAT_ENTRY_FREE)
                return i;
    }
    printf("[get_free_fat_cluster] no free cluster\n");
    return 0;
}

/**
 * input: relative cluster number
 * output: value at FAT table entry
 */
unsigned int read_fat_entry(unsigned int cluster)
{
    unsigned int *fat_table, block_idx = 0;
    char buf[BLOCK_SIZE];

    block_idx = fat_info.fat_region + cluster / (BLOCK_SIZE / sizeof(unsigned int));
    readblock(block_idx, buf);
    fat_table = (unsigned int *)buf;
    return fat_table[cluster % (BLOCK_SIZE / sizeof(unsigned int))];
}

/* Write values at FAT table entry according cluster number */
void write_fat_table(unsigned int cluster, unsigned int value)
{
    unsigned int *fat_table, block_idx = 0;
    char buf[BLOCK_SIZE];

    block_idx = fat_info.fat_region + cluster / (BLOCK_SIZE / sizeof(unsigned int));
    readblock(block_idx, buf);
    fat_table = (unsigned int *)buf;
    fat_table[cluster % (BLOCK_SIZE / sizeof(unsigned int))] |= value;
    writeblock(block_idx, buf);
}

/* Get FAT32 boot sector and extract meta data to fat_info. */
void get_fat32_boot_sector(void)
{
    char buf[512];
    struct partition_table_entry *partition;

    /* Get partition information from MBR */
    get_mbr();
    partition = &mbr.partitions[0];
    fat_info.starting_sector = partition->starting_sector;

    /* Get FAT boot sector */
    readblock(fat_info.starting_sector, buf);
    memcpy(&fat_boot_sector, buf, sizeof(struct fat_boot_sector));
    if (strcmp((char *)fat_boot_sector.fs_type, "FAT32   "))
        printf("[get_fat32_boot_sector] partition 1 is not FAT32");

    /* Update fat_info*/
    fat_info.fat_region = fat_info.starting_sector + fat_boot_sector.nr_reserved_sectors;
    fat_info.data_region = fat_info.fat_region + (fat_boot_sector.nr_fat_copies * fat_boot_sector.sectors_per_fat);
    fat_info.blocks_per_cluster = fat_boot_sector.sectors_per_cluster;
    fat_info.sectors_per_fat = fat_boot_sector.sectors_per_fat;
    fat_info.sectors_per_cluster = fat_boot_sector.sectors_per_cluster;
    fat_info.file_name_type = mbr.partitions[0].partition_type; // 0xb for SFN, 0xc for LFN

    print_fat32_boot_sector();
}

/**
 * Setup all the vnodes and inodes under FAT32 file system.
 * dir_inode->data should be the directory table.
 */
void setup_fat32(struct fat32_inode *dir_inode)
{
    struct fat32_inode *child_inode;
    char buf[BLOCK_SIZE];
    char file_name[MAX_FILE_NAME_LEN];
    int i, j, idx, child_idx = 0;
    unsigned int child_cluster;
    struct sfn_entry *dir_table;

    unsigned int cur_entry = dir_inode->cluster, nxt_entry = read_fat_entry(cur_entry);

    while (IS_FAT_ENTRY_VALID(cur_entry)) {
        /* create vnode for all the file in root directory */
        readblock(fat_info.data_region + (cur_entry - 2) * fat_info.blocks_per_cluster, dir_inode->data);
        dir_table = (struct sfn_entry *)dir_inode->data;
        // dir_table = (struct sfn_entry *)dir_inode->data;
        for (i = 0; i < (BLOCK_SIZE / DIR_ENTRY_SIZE); i++) {
            idx = 0;
            child_cluster = 0;

            /* Skip hidden file and LFN files */
            if ((dir_table[i].attr & ATTR_HIDDEN) || (dir_table[i].attr & 0xf) == ATTR_LONG_NAME)
                continue;
            /* Skip `.` and `..` file name */
            if (dir_table[i].name[0] == ' ' || dir_table[i].name[0] == '\0' || dir_table[i].name[0] == '.')
                continue;

            /* Check whether the file is directory and parse its name */
            if (dir_table[i].attr & ATTR_DIRECTORY) {
                dir_inode->childs[child_idx] = fat32_create_vnode(0, FSNODE_TYPE_DIR);

                for (j = 0; j < 8; j++)
                    if (dir_table[i].name[j] != ' ')
                        file_name[idx++] = dir_table[i].name[j];
                file_name[idx] = '\0';
            } else {
                dir_inode->childs[child_idx] = fat32_create_vnode(0, FSNODE_TYPE_FILE);
                
                /* Parse the file name under root directory */
                for (j = 0; j < 8; j++)
                    if (dir_table[i].name[j] != ' ')
                        file_name[idx++] = dir_table[i].name[j];
                file_name[idx++] = '.';
                for (j = 0; j < 3; j++)
                    if (dir_table[i].file_ext[j] != ' ')
                        file_name[idx++] = dir_table[i].file_ext[j];
                file_name[idx] = '\0';
            }

            child_inode = dir_inode->childs[child_idx]->internal;

            /* Copy the file name to child inode */
            strcpy(child_inode->name, file_name);
            /* Parse the cluster number of file */
            child_cluster = dir_table[i].cluster_high << 16 | dir_table[i].cluster_low;
            child_inode->cluster = child_cluster;
            child_inode->abs_block_num = fat_info.data_region + (child_cluster - 2) * fat_info.blocks_per_cluster;
            /* Copy the content of child file from SD */
            readblock(child_inode->abs_block_num, buf);
            memcpy(child_inode->data, buf, BLOCK_SIZE);
            /* Get the file size */
            child_inode->data_size = dir_table[i].file_size;

#ifdef VFS_DEBUG
            printf("[setup_fat32] file name: %s, type %d, cluster: %d, abs_block_num: %d, size: %d\n",
                child_inode->name, child_inode->type, child_inode->cluster, child_inode->abs_block_num, child_inode->data_size);
#endif

            /* Recursively setup the vnode and inode. Not test its function */
            if (child_inode->type == FSNODE_TYPE_DIR)
                setup_fat32(child_inode);

            child_idx++;
        }
        
        cur_entry = nxt_entry;
        nxt_entry = read_fat_entry(nxt_entry);
    }
}

void print_fat32_boot_sector(void)
{
    printf("partition type            %x\n", mbr.partitions[0].partition_type);
    printf("bytes per sector          %d\n", fat_boot_sector.bytes_per_sector);
    printf("sector per cluster        %d\n", fat_boot_sector.sectors_per_cluster);
    printf("root dir                  %d\n", fat_boot_sector.root_dir);
    printf("number of reserved sector %d\n", fat_boot_sector.nr_reserved_sectors);
    printf("number of FAT table       %d\n", fat_boot_sector.nr_fat_copies);
    printf("sectors per FAT table     %d\n\n", fat_boot_sector.sectors_per_fat);
}

/* Given relative cluster number, print directory table contents. */
void print_fat32_dir(unsigned int cluster)
{
    char *tmp_buf, buf[BLOCK_SIZE];
    unsigned int c, i;
    struct sfn_entry *dir_table;
    unsigned int cur_entry = cluster, nxt_entry = read_fat_entry(cur_entry);

    while (IS_FAT_ENTRY_VALID(cur_entry)) {
        printf("current cluster %d %x\n", cur_entry, cur_entry);
        printf("next cluster    %d %x\n", nxt_entry, nxt_entry);
        readblock(fat_info.data_region + (cur_entry - 2) * fat_info.blocks_per_cluster, buf);
        dir_table = (struct sfn_entry *)buf;
        for (i = 0; i < BLOCK_SIZE / DIR_ENTRY_SIZE; i++) {
            c = dir_table[i].cluster_high << 16 | dir_table[i].cluster_low;
            tmp_buf = (char *)&dir_table[i];
            for (int k = 0; k < 11; k++)
                printf("%c. ", tmp_buf[k]);
            printf("attr %x, cluster %d, file size %d.", dir_table[i].attr, c, dir_table[i].file_size);
            printf("\n");
        }
        printf("----next block of this directory\n");
        cur_entry = nxt_entry;
        nxt_entry = read_fat_entry(nxt_entry);
    }

    printf("[print_dir_table] end of root directory\n");
    return;
}

void fat32_new_dir_entry(unsigned int dir_cluster, struct sfn_entry *new_entry)
{
    char buf[BLOCK_SIZE];
    unsigned int i;
    struct sfn_entry *dir_table;
    unsigned int cur_entry = dir_cluster, nxt_entry = read_fat_entry(cur_entry);

    while (IS_FAT_ENTRY_VALID(cur_entry)) {
#ifdef VFS_DEBUG
        printf("[fat32_new_dir_entry] cur_cluster %d %x\n", cur_entry, cur_entry);
        printf("[fat32_new_dir_entry] nxt_cluster %d %x\n", nxt_entry, nxt_entry);
#endif
        readblock(fat_info.data_region + (cur_entry - 2) * fat_info.blocks_per_cluster, buf);
        dir_table = (struct sfn_entry *)buf;
        for (i = 0; i < BLOCK_SIZE / DIR_ENTRY_SIZE; i++) {
            if (dir_table[i].attr != 0)
                continue;
            // memcpy(&dir_table[i], new_entry, sizeof(struct sfn_entry));
            dir_table[i] = *new_entry;
            printf("new entry success\n");
            writeblock(fat_info.data_region + (cur_entry - 2) * fat_info.blocks_per_cluster, (char *)dir_table);
            return;
        }
        cur_entry = nxt_entry;
        nxt_entry = read_fat_entry(nxt_entry);
    }

    printf("[fat32_new_dir_entry] No space to new entry in dir.\n");
    return;
}