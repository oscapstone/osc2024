
#include "fat32fs.h"
#include "mm/mm.h"
#include "utils/printf.h"
#include "dev/mbr.h"
#include "dev/sdhost.h"
#include "utils/utils.h"

/**
 * Load the content from hardware
*/
static int init_vnode(FS_VNODE* vnode);
/**
 * parse the directory content 
*/
static int parse_dir_entries(FS_VNODE* node);

static int write(FS_FILE *file, const void *buf, size_t len);
static int read(FS_FILE *file, void *buf, size_t len);
static int open(FS_VNODE *file_node, FS_FILE **target);
static int close(FS_FILE *file);
static long lseek64(FS_FILE *file, long offset, int whence);
static int ioctl(FS_FILE *file, unsigned long request, ...);

FS_FILE_OPERATIONS fat32fs_f_ops = {
    write,
    read,
    open,
    close,
    lseek64,
    ioctl
};

static int lookup(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name);
static int create(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name);
static int mkdir(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name);
FS_VNODE_OPERATIONS fat32fs_v_ops = {
    lookup,
    create,
    mkdir
};

/**
 * MOunt the file system, need to give the hardware RW function first
*/
static int setup_mount(FS_FILE_SYSTEM* fs, FS_MOUNT* mount) {

    if (utils_strncmp(fs->name, FAT32_FS_NAME, utils_strlen(FAT32_FS_NAME)) != 0) {
        return -1;
    }

    if (!mount->read & !mount->write) {
        printf("[FS][ERROR] FAT32fs: Does not have R/W function implement, cannot mount.\n");
        return -1;
    }
    MBR_INFO mbr;

    // TODO: parse MBR to get the partition
    printf("[SDCARD] Reading MBR info...\n");
    mount->read(0, &mbr, sizeof(MBR_INFO));
    if (mbr.magic_code[0] != 0x55 || mbr.magic_code[1] != 0xaa) {
        NS_DPRINT("[SDCARD][ERROR] Failed to load MBR structure.\n");
        return -1;
    }

    if (mount->internal) {
        kfree(mount->internal);
    }

    mount->internal = kzalloc(sizeof(FAT32_FS_INTERNAL));

    FAT32_FS_INTERNAL* internalData = (FAT32_FS_INTERNAL*)mount->internal;

    // Just use first partition mount to system
    // read the first partition boot sector
    PARTITION_ENTRY* part0 = &mbr.pe[0];
    printf("Partition 0 info addr: %p\n", part0);
    U32 boot_sector_index = utils_read_unaligned_u32((void*)&part0->start_sector);  // read this because of arm trash memory load store need to be 4 byte align
    NS_DPRINT("[FS][TRACE] Partition 0 boot sector index: %d\n", boot_sector_index);
    // boring, can check the fs_flags it is FAT32 or not
    mount->read(boot_sector_index * MBR_DEFAULT_SECTOR_SIZE, (void*)&internalData->bpb, sizeof(FAT32_BPB));

    if (utils_strncmp((const char*)internalData->bpb.fs_type, FAT32_TYPE_NAME, FAT32_TYPE_NAME_LEN) != 0) {
        printf("[FS][ERROR] FAT32fs: fat label is not correct.\n");
        return -1;
    }

    // 
    internalData->fat_start_sector = boot_sector_index + internalData->bpb.reserved_sectors;
    internalData->data_start_sector = internalData->fat_start_sector + internalData->bpb.num_fat * internalData->bpb.sectors_per_fat32 - 2/* I have no idea why it should subtract by 2 */;

    NS_DPRINT("[FS][TRACE] FAT table start sector: %d\n", internalData->fat_start_sector);
    NS_DPRINT("[FS][TRACE] Data start sector: %d\n", internalData->data_start_sector);

    // copy the FAT table to memory
    internalData->sector_per_fat = utils_read_unaligned_u32(&internalData->bpb.sectors_per_fat32);
    U32 num_sector_fat_table = internalData->bpb.num_fat * internalData->sector_per_fat;

    internalData->bytes_per_sector = utils_read_unaligned_u16(&internalData->bpb.bytes_per_sector);
    internalData->fat_table = kzalloc(num_sector_fat_table * internalData->bytes_per_sector);
    mount->read(internalData->fat_start_sector * MBR_DEFAULT_SECTOR_SIZE, internalData->fat_table, num_sector_fat_table * internalData->bytes_per_sector);
    NS_DPRINT("[FS] FAT32fs: memory FAT table addr: %p\n",  internalData->fat_table);

    NS_DPRINT("[FS] FAT32fs: mounting on %s\n", mount->root->name);

    mount->root->mount = mount;
    mount->root->f_ops = &fat32fs_f_ops;
    mount->root->v_ops = &fat32fs_v_ops;
    mount->fs = fs;

    link_list_init(&mount->root->childs);
    mount->root->child_num = 0;
    mount->root->content = NULL;
    mount->root->content_size = 0;

    FAT32_VNODE_INTERNAL* root_internal = kzalloc(sizeof(FAT32_VNODE_INTERNAL));

    mount->root->internal = root_internal;

    root_internal->block_id = internalData->bpb.root_cluster;      // BPB indicate the root first cluster (block)
    NS_DPRINT("Root start cluster id: %d\n", root_internal->block_id);

    // initialize the root node
    if (init_vnode(mount->root) == -1) {
        printf("[FS][ERROR] FAT32fs: Failed to initialize the root node.\n");
        return -1;
    }
    parse_dir_entries(mount->root);

    // make sure that root is not a regular file in vfs
    mount->root->mode &= ~(S_IFREG);
    mount->root->mode |= S_IFDIR;

    return 0;
}

/**
 * @param index
 *      index in FAT table
*/
static U32 get_chain_len(FS_VNODE* vnode) {
    FS_MOUNT* mount = vnode->mount;
    FAT32_FS_INTERNAL* mount_internal = (FAT32_FS_INTERNAL*)mount->internal;

    U32 index = ((FAT32_VNODE_INTERNAL*)vnode->internal)->block_id;

    U32 len = 1;

    while (1) {
        // get the next index
        index = mount_internal->fat_table[index];
        
        if (index >= FAT32_FAT_EOF_START && index <= FAT32_FAT_EOF_END) {
            return len;
        }
        if (index == FAT32_FAT_BAD) {
            printf("Bad cluster.\n");
            return len;
        }

        len++;
    }

    // dont go here
    return 0;
}

/**
 * Read the data from hardware
*/
static int init_vnode(FS_VNODE* vnode) {
    FAT32_FS_INTERNAL* mount_internal = (FAT32_FS_INTERNAL*)vnode->mount->internal;
    FAT32_VNODE_INTERNAL* node_internal = (FAT32_VNODE_INTERNAL*)vnode->internal;

    // get the length of the chain (measure buffer to read)
    U32 num_blocks = get_chain_len(vnode);
    U32 size = num_blocks * utils_read_unaligned_u16(&mount_internal->bytes_per_sector);
    void* buf = kzalloc(size);
    U32 index = node_internal->block_id;
    NS_DPRINT("[FS] FAT32fs: init_vnode(): start index: %d\n", index);
    for (U32 i = 0; i < num_blocks; i++) {
        vnode->mount->read((mount_internal->data_start_sector + index) * mount_internal->bytes_per_sector, ((char*)buf + (i * mount_internal->bytes_per_sector)), mount_internal->bytes_per_sector);
        index = mount_internal->fat_table[index];
        NS_DPRINT("[FS] FAT32fs: init_vnode(): next index: %d\n", index);
    }
    node_internal->num_block = num_blocks;
    if (vnode->content) {
        kfree(vnode->content);
    }
    vnode->content = buf;
    vnode->content_size = size;
    node_internal->is_dirty = FALSE;
    node_internal->is_loaded = TRUE;

    return 0;
}

FS_FILE_SYSTEM* fat32fs_create() {
    FS_FILE_SYSTEM* fs = kmalloc(sizeof(FS_FILE_SYSTEM));
    fs->name = FAT32_FS_NAME;
    fs->setup_mount = &setup_mount;
    link_list_init(&fs->list);
    return fs;
}

static int open(FS_VNODE* node, FS_FILE** target) {
    if (init_vnode(node) == -1) {
        return -1;
    }
    return 0;
}

static int close(FS_FILE* file) {
    return 0;
}

static int read(FS_FILE* file, void* buf, size_t len) {
    FS_VNODE* node = file->vnode;
    if (!S_ISREG(node->mode)) {
        return -1;
    }
    len = (len > node->content_size - file->pos) ? node->content_size - file->pos : len;
    if (len == 0) {
        return -1;
    }
    // TODO
    memcpy((void*) ((UPTR)file->vnode->content + file->pos), buf, len);
    file->pos += len;
    return len;
}

static int write(FS_FILE *file, const void *buf, size_t len) {
    // TODO
    return -1;
}

static long lseek64(FS_FILE *file, long offset, int whence) {
    switch (whence)
    {
    case SEEK_SET:
        file->pos = offset;
        return offset;
        break;
    default:
        return -1;
        break;
    }
}

/**
*/
static int lookup(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name) {
    if (!dir_node) {
        return -1;
    }

    if (!S_ISDIR(dir_node->mode)) {
        printf("[FS][ERROR] FAT32fs: lookup(): directory node is not a directory. name: %s\n", dir_node->name);
        return -1;
    }

    // no children, try to parse it
    if (!dir_node->child_num) {
        parse_dir_entries(dir_node);
    }

    FAT32_VNODE_INTERNAL* node_internal = (FAT32_VNODE_INTERNAL*)dir_node->internal;
    
    FS_VNODE* vnode = NULL;
    LLIST_FOR_EACH_ENTRY(vnode, &dir_node->childs, self) {
        if (utils_strncmp(vnode->name, component_name, utils_strlen(vnode->name)) == 0) {
            // found and if not loaded. load the vnode content
            if (!vnode->content) {
                init_vnode(vnode);
            }
            *target = vnode;
            return 0;
        }
    }

    // not found
    return -1;
}

static int create(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name) {
    // TODO
    return -1;
}

static int mkdir(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name) {
    // TODO
    return -1;

}

static int ioctl(FS_FILE *file, unsigned long request, ...) {
    return -1;
}

/**
 * @return
 *      -1 mean no name
*/
static int convert_fat_name(const FAT32_DIR_ENTRY* entry, char* dst) {
    size_t offset = 0;
    const char* fat_name = entry->name;
    for (int i = 0; i < 8; i++) {
        if (fat_name[i] == ' ' || fat_name[i] == '\0')
            break;
        dst[offset++] = fat_name[i];
    }
    if (offset == 0)
        return -1;
    dst[offset++] = '.';
    const char* ext = entry->ext;
    for (int i = 0; i < 3; i++) {
        if (ext[i] == ' ')
            break;
        dst[offset++] = ext[i];
    }
    dst[offset] = '\0';
    NS_DPRINT("ori file name:%s\n", entry->name);
    NS_DPRINT("ori ext name: %s\n", entry->ext);
    NS_DPRINT("convert name: %s\n", dst);
    return 0;
}

/**
 * Assume content is loaded
*/
static int parse_dir_entries(FS_VNODE* node) {
    if (!node->content) {
        NS_DPRINT("[FS][ERROR] FAT32fs: File content is not loaded. can not parse dir entries.\n");
        return -1;
    }

    FAT32_VNODE_INTERNAL* node_internal = (FAT32_VNODE_INTERNAL*)node->internal;

    FAT32_DIR_ENTRY* entry = (FAT32_DIR_ENTRY*)node->content;
    UPTR end_ptr = (UPTR)node->content + node->content_size;
    while ((UPTR) entry < end_ptr) {
        char tmp_name[13];
        
        // no more file to parse
        if (convert_fat_name(entry, tmp_name) == -1) {
            break;
        }

        FS_VNODE* new_node = vnode_create(tmp_name, 0);
        
        // is a directory child
        if (entry->attribute & FAT32_DIR_ENTRY_ATTR_DIR) {
            new_node->mode = S_IFDIR;
        } else {
            new_node->mode = S_IFREG;
        }

        // internal modification
        new_node->internal = kzalloc(sizeof(FAT32_VNODE_INTERNAL));
        FAT32_VNODE_INTERNAL* new_internal = (FAT32_VNODE_INTERNAL*) new_node->internal;
        new_internal->block_id = (utils_read_unaligned_u16(&entry->high_block_index) << 16) + utils_read_unaligned_u16(&entry->low_block_index);
        NS_DPRINT("[FS] FAT32fs: block id: %d\n", new_internal->block_id);
        new_internal->is_dirty = FALSE;
        new_internal->is_loaded = FALSE;    // not loaded to faster the searching when lookup
        
        new_node->content_size = entry->file_size;

        // common modification
        new_node->mount = node->mount;
        new_node->v_ops = node->v_ops;
        new_node->f_ops = node->f_ops;
        new_node->parent = node;

        link_list_push_back(&node->childs, &new_node->self);
        node->child_num++;

        entry++;
    }

    return 0;
}