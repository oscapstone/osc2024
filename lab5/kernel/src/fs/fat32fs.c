
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

static int find_next_free_cluster_id(FAT32_FS_INTERNAL* fs_internal) {
    U32 index = fs_internal->next_free_cluster;
    if (index == FAT32_FAT_BAD) {
        index = 2;      // find from root
    }

    U32 number_of_fat_entries = fs_internal->fat_table_size / sizeof(U32);

    while (index < number_of_fat_entries) {
        if (fs_internal->fat_table[index] == 0) {
            fs_internal->fat_table[index] = FAT32_FAT_BAD;          // label it, use it later
            return index;
        }
        index++;
    }

    return FAT32_FAT_BAD;       // bad not found
}

static int find_free_cluster_id(FAT32_FS_INTERNAL* fs_internal) {
    NS_DPRINT("Finding free cluster id...\n");
    U32 cluster_id = fs_internal->next_free_cluster;

    if (cluster_id > 2 && cluster_id < FAT32_FAT_BAD) {
        fs_internal->next_free_cluster = find_next_free_cluster_id(fs_internal);
        return cluster_id;    
    }
    NS_DPRINT("[FS] FAT32fs: next free cluster is not initialized.\n");
    fs_internal->next_free_cluster = find_next_free_cluster_id(fs_internal);
    if (fs_internal->next_free_cluster == FAT32_FAT_BAD) {
        NS_DPRINT("[FS] FAT32fs: can not find free cluster id.\n");
        return FAT32_FAT_BAD;
    }
    NS_DPRINT("[FS] FAT32fs: next free cluster initialized.\n");
    cluster_id = find_next_free_cluster_id(fs_internal);

    return cluster_id;
}

static void write_node(FS_VNODE* node) {
    FAT32_FS_INTERNAL* fs_internal = (FAT32_FS_INTERNAL*)node->mount->internal;
    FAT32_VNODE_INTERNAL* node_internal = (FAT32_VNODE_INTERNAL*) node->internal;

    size_t offset = 0;

    // update parent entry file size, because the child will update first so when parent update the entry info is updated.
    FS_VNODE* parent_node = node->parent;
    FAT32_DIR_ENTRY* dir_entry = &((FAT32_DIR_ENTRY*) parent_node->content)[node_internal->parent_dir_entry_index];
    dir_entry->file_size = node->content_size;

    U32 index = node_internal->cluster_id;
    if (index == FAT32_FAT_BAD) {       // is not allocated in FS
        index = find_free_cluster_id(fs_internal);
        node_internal->cluster_id = index;
    }
    dir_entry->high_block_index = (node_internal->cluster_id >> 16) & 0xffff;
    dir_entry->low_block_index = (node_internal->cluster_id) & 0xffff;
    while (offset < node->content_size) {
        U32 size = (offset + fs_internal->bytes_per_sector) > node->content_size ? node->content_size - offset : fs_internal->bytes_per_sector;

        // write data
        size_t write_offset = (fs_internal->data_start_sector + index) * fs_internal->bytes_per_sector;
        NS_DPRINT("[FS] FAT32fs: write to HW, addr: 0x%x\n", write_offset);
        node->mount->write(write_offset, (UPTR) node->content + offset, size);

        U32 next_index = fs_internal->fat_table[index];
        
        if (next_index >= FAT32_FAT_BAD && next_index <= FAT32_FAT_EOF_END && offset < node->content_size) {
            next_index = find_free_cluster_id(fs_internal);
            if (next_index <= 2 || next_index >= FAT32_FAT_BAD) {
                printf("[FS][ERROR] FAT32fs: Failed to write data to file.\n");
                return;
            }
            fs_internal->fat_table[index] = next_index;        // make next sector end.
        }

        index = fs_internal->fat_table[index];
        offset += size;
    }

}

static void check_node(FS_VNODE* node) {
    
    // check child first
    FS_VNODE* child_node = NULL;
    LLIST_FOR_EACH_ENTRY(child_node, &node->childs, self) {
        check_node(child_node);
    }

    NS_DPRINT("[FS] FAT32fs: checking virtual node: %s\n", node->name);
    FAT32_VNODE_INTERNAL* node_internal = (FAT32_VNODE_INTERNAL*) node->internal;
    if (node_internal->is_dirty) {
        NS_DPRINT("[FS] FAT32fs: node is dirty. syncing...\n");
        write_node(node);
        node_internal->is_dirty = FALSE;
    }
}

// sync the external storage with FAT32
static void sync(FS_MOUNT* mount) {
    NS_DPRINT("[FS] FAT32fs: syncing...\n");
    FAT32_FS_INTERNAL* fs_internal = (FAT32_FS_INTERNAL*)mount->internal;

    FS_VNODE* node = mount->root;

    check_node(node);

    // just write the first FAT table
    mount->write(fs_internal->fat_start_sector * MBR_DEFAULT_SECTOR_SIZE, fs_internal->fat_table, fs_internal->fat_table_size);
}

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
    U32 boot_sector_index = utils_read_unaligned_u32((void*)&part0->start_sector);  // read this because of trash arm memory load store need to be 4 byte align
    NS_DPRINT("[FS][TRACE] Partition 0 boot sector index: %d\n", boot_sector_index);
    // boring, can check the fs_flags it is FAT32 or not
    mount->read(boot_sector_index * MBR_DEFAULT_SECTOR_SIZE, (void*)&internalData->bpb, sizeof(FAT32_BPB));

    if (utils_strncmp((const char*)internalData->bpb.fs_type, FAT32_TYPE_NAME, FAT32_TYPE_NAME_LEN) != 0) {
        printf("[FS][ERROR] FAT32fs: fat label is not correct.\n");
        return -1;
    }

    internalData->sector_per_fat = utils_read_unaligned_u32(&internalData->bpb.sectors_per_fat32);
    internalData->bytes_per_sector = utils_read_unaligned_u16(&internalData->bpb.bytes_per_sector);
    // 
    internalData->fat_start_sector = boot_sector_index + internalData->bpb.reserved_sectors;
    internalData->data_start_sector = internalData->fat_start_sector + internalData->bpb.num_fat * internalData->sector_per_fat - 2/* I have no idea why it should subtract by 2 */;

    NS_DPRINT("[FS][TRACE] FAT table start sector: %d\n", internalData->fat_start_sector);
    NS_DPRINT("[FS][TRACE] Data start sector: %d\n", internalData->data_start_sector);

    // copy the FAT table to memory
    U32 num_sector_fat_table = internalData->bpb.num_fat * internalData->sector_per_fat;

    internalData->fat_table_size = internalData->sector_per_fat * internalData->bytes_per_sector;
    internalData->fat_table = kzalloc(num_sector_fat_table * internalData->bytes_per_sector);
    mount->read(internalData->fat_start_sector * MBR_DEFAULT_SECTOR_SIZE, internalData->fat_table, num_sector_fat_table * internalData->bytes_per_sector);
    NS_DPRINT("[FS] FAT32fs: memory FAT table addr: %p\n",  internalData->fat_table);

    // TODO parse FSInfo
    internalData->next_free_cluster = FAT32_FAT_BAD;

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

    root_internal->cluster_id = internalData->bpb.root_cluster;      // BPB indicate the root first cluster (block)
    NS_DPRINT("Root start cluster id: %d\n", root_internal->cluster_id);

    // initialize the root node
    if (init_vnode(mount->root) == -1) {
        printf("[FS][ERROR] FAT32fs: Failed to initialize the root node.\n");
        return -1;
    }

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

    U32 index = ((FAT32_VNODE_INTERNAL*)vnode->internal)->cluster_id;

    U32 len = 1;

    while (1) {
        // get the next index
        index = mount_internal->fat_table[index];
        
        if (index < 2) {
            return 0;
        }

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
        
    if (num_blocks == 0) {
        return -1;
    }

    U32 size = num_blocks * mount_internal->bytes_per_sector;
    void* buf = kzalloc(size);
    U32 index = node_internal->cluster_id;
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
    if (S_ISDIR(vnode->mode)) {
        parse_dir_entries(vnode);
        vnode->content_size = size;
        // vnode->content = NULL;
        // vnode->content_size = 0;
    } else if (S_ISREG(vnode->mode)) {
        //
    }

    node_internal->is_dirty = FALSE;
    node_internal->is_loaded = TRUE;

    return 0;
}

FS_FILE_SYSTEM* fat32fs_create() {
    FS_FILE_SYSTEM* fs = kmalloc(sizeof(FS_FILE_SYSTEM));
    fs->name = FAT32_FS_NAME;
    fs->setup_mount = &setup_mount;
    fs->sync = &sync;
    link_list_init(&fs->list);
    return fs;
}

static int open(FS_VNODE* node, FS_FILE** target) {
    // load the node when open
    FAT32_VNODE_INTERNAL* internal = (FAT32_VNODE_INTERNAL*) node->internal;
    if (!internal->is_loaded) {
        if (init_vnode(node) == -1) {
            return -1;
        }
    }
    return 0;
}

static int close(FS_FILE* file) {
    // lab require only when sync() called. write the change back to external storage.
    return 0;
}

static int read(FS_FILE* file, void* buf, size_t len) {
    FS_VNODE* node = file->vnode;

    FAT32_VNODE_INTERNAL* internal = (FAT32_VNODE_INTERNAL*)node->internal;

    if (!internal->is_loaded) {
        NS_DPRINT("[FS][ERROR] FAT32fs: read() wired, reading a file while not load yet.\n");
        return -1;
    }

    if (!S_ISREG(node->mode)) {
        NS_DPRINT("[FS][ERROR] FAT32fs: read() file is not regular.\n");
        return -1;
    }
    len = (len > node->content_size - file->pos) ? node->content_size - file->pos : len;
    if (len == 0) {
        NS_DPRINT("[FS][ERROR] FAT32fs: read(): read length is 0.\n");
        return -1;
    }
    // TODO
    memcpy((void*) ((UPTR)file->vnode->content + file->pos), buf, len);
    file->pos += len;
    //NS_DPRINT("[FS] FAT32fs: read() read %d bytes\n", len);
    return len;
}

static int write(FS_FILE *file, const void *buf, size_t len) {
    FS_VNODE* vnode = file->vnode;
    if (!S_ISREG(vnode->mode)) {
        NS_DPRINT("[FS] FAT32fs: write(): not a regular file.\n");
        return -1;
    }
    if (vnode->content_size <= file->pos + len) {
        size_t new_size = file->pos + len + 1;
        void* new_content = kzalloc(new_size);
        if (vnode->content) {
            memcpy(vnode->content, new_content, vnode->content_size);
            kfree(vnode->content);
        }
        vnode->content = new_content;
        vnode->content_size = new_size;
    }

    memcpy(buf, (void*)((char*) vnode->content + file->pos), len);
    file->pos += len;
    FAT32_VNODE_INTERNAL* node_internal = (FAT32_VNODE_INTERNAL*) vnode->internal;
    node_internal->is_dirty = TRUE;

    return len;
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

static int register_dir_entry(FS_VNODE* dir_node, const char* component_name, U8 attribute, U32* dir_entry_index) {
    FAT32_FS_INTERNAL* fs_internal = (FAT32_FS_INTERNAL*) dir_node->mount->internal;

    // REGISTER the entry in parent directory
    // try finding empty slot in directory
    NS_DPRINT("Try finding parent dir entry.\n");
    BOOL found_entry = FALSE;
    FAT32_DIR_ENTRY* dir_entries = (FAT32_DIR_ENTRY*) dir_node->content;
    U32 entry_index = 0;
    U32 number_of_entries = dir_node->content_size / sizeof(FAT32_DIR_ENTRY);
    FAT32_DIR_ENTRY* dir_entry = NULL;
    while (entry_index < number_of_entries) {
        dir_entry = &dir_entries[entry_index];
        //printf("[FS] dir name: %x\n", dir_entry->name[0]);
        if (dir_entry->name[0] == FAT32_DIR_ENTRY_UNUSED || dir_entry->name[0] == FAT32_DIR_ENTRY_LAST_AND_UNUSED) {
            found_entry = TRUE;
            break;
        }
        entry_index++;
    }
    if (found_entry == FALSE) {     // enlarge the 
        NS_DPRINT("[FS] FAT32fs: directory is full, enlarge it.\n");
        size_t new_size = dir_node->content_size + fs_internal->bytes_per_sector;
        void* new_buf = kmalloc(new_size);
        if (dir_node->content) {
            memcpy(dir_node->content, new_buf, dir_node->content_size);
            kfree(dir_node->content);
        }
        FAT32_DIR_ENTRY* new_entries = (FAT32_DIR_ENTRY*) ((UPTR)new_buf + dir_node->content_size);
        for (U32 i = 1; i < 16; i++) {
            new_entries[i].name[0] = FAT32_DIR_ENTRY_UNUSED;    // mark other 
        }    
        dir_node->content = new_buf;
        dir_node->content_size = new_size;

        dir_entry = &new_entries[0];
    }
    U32 name_offset = 0;
    U32 dir_name_offset = 0;
    for (; dir_name_offset < 8; dir_name_offset++) {
        dir_entry->name[dir_name_offset] = ' ';   // make sure
        if (component_name[name_offset] == '.') {
            name_offset++;          // skip . 
            break;
        }
        
        dir_entry->name[dir_name_offset] = component_name[name_offset++];
    }
    for (; dir_name_offset < 8; dir_name_offset++) {
        dir_entry->name[dir_name_offset] = ' ';   // make sure
    }
#ifdef NS_DEBUG
    for (U32 i = 0; i < 8; i++) {
        NS_DPRINT("[FS] FAT32fs: dir name: %c\n", dir_entry->name[i]);
    }
#endif

    dir_entry->ext[0] = ' ';
    dir_entry->ext[1] = ' ';
    dir_entry->ext[2] = ' ';
    for(U32 i = 0; i < 3; i++) {
        if (component_name == ' ') {
            break;
        }

        dir_entry->ext[i] = component_name[name_offset++];
    }

    dir_entry->attribute = attribute;
    // update in sync
    //dir_entry->high_block_index = (cluster_id >> 16) & 0xffff;
    //dir_entry->low_block_index = cluster_id & 0xffff;
    dir_entry->file_size = 0;

    *dir_entry_index = entry_index;

    // mark parent dir dirty
    FAT32_VNODE_INTERNAL* parent_internal = (FAT32_VNODE_INTERNAL*) dir_node->internal;
    parent_internal->is_dirty = TRUE;

    return 0;
}

static int create(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name) {
    if (lookup(dir_node, target, component_name) == 0) {
        printf("[FS] FAT32fs: create(): file is already exist. name: %d\n", component_name);
        return -1;
    }
    FAT32_FS_INTERNAL* fs_internal = (FAT32_FS_INTERNAL*) dir_node->mount->internal;

    U32 entry_index;
    NS_DPRINT("[FS] FAT32fs: Registering directory entry...\n");
    if (register_dir_entry(dir_node, component_name, 0, &entry_index) == -1) {
        return -1;
    }
    NS_DPRINT("[FS] FAT32fs: parent dir entry: %u\n", entry_index);

    FS_VNODE* new_vnode = vnode_create(component_name, S_IFREG);
    new_vnode->mount = dir_node->mount;
    new_vnode->v_ops = dir_node->v_ops;
    new_vnode->f_ops = dir_node->f_ops;
    new_vnode->parent = dir_node;

    new_vnode->internal = kzalloc(sizeof(FAT32_VNODE_INTERNAL));
    FAT32_VNODE_INTERNAL* internal = (FAT32_VNODE_INTERNAL*) new_vnode->internal;
    internal->is_dirty = TRUE;
    internal->cluster_id = find_free_cluster_id(fs_internal);           // update on sync 
    internal->parent_dir_entry_index = entry_index;

    link_list_push_back(&dir_node->childs, &new_vnode->self);
    dir_node->child_num++;

    *target = new_vnode;
    return 0;
}

static int mkdir(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name) {

    //NS_DPRINT("[FS][TRACE] rootfs: mkdir(): creating %s dir\n", component_name);
    if (!lookup(dir_node, target, component_name)) {
        printf("[FS] rootfs: mkdir(): directory is already exist. name: %d\n", component_name);
        return -1;
    }
    FAT32_FS_INTERNAL* fs_internal = dir_node->mount->internal;

    // REGISTER the entry in parent directory
    U32 entry_index;
    if (register_dir_entry(dir_node, component_name, FAT32_DIR_ENTRY_ATTR_DIR, &entry_index) == -1) {
        return -1;
    }

    FS_VNODE* new_vnode = vnode_create(component_name, S_IFDIR);
    new_vnode->mount = dir_node->mount;
    new_vnode->v_ops = dir_node->v_ops;
    new_vnode->f_ops = dir_node->f_ops;
    new_vnode->parent = dir_node;

    new_vnode->internal = kzalloc(sizeof(FAT32_VNODE_INTERNAL));
    FAT32_VNODE_INTERNAL* internal = (FAT32_VNODE_INTERNAL*) new_vnode->internal;
    internal->is_dirty = TRUE;
    internal->parent_dir_entry_index = entry_index;

    internal->cluster_id = find_free_cluster_id(fs_internal);   // update in FAT32

    link_list_push_back(&dir_node->childs, &new_vnode->self);
    dir_node->child_num++;

    *target = new_vnode;
    //NS_DPRINT("[FS][TRACE] rootfs: mkdir(): dir %s created.\n", component_name);
    return 0;
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
    if (fat_name[0] == ' ')
        return -1;
    for (int i = 0; i < 8; i++) {
        if (fat_name[i] == ' ')
            break;

        // illegal name
        if (fat_name[i] < ' ' || fat_name[i] > 0x7E)
            return -1;
        if (fat_name[i] >= 'a' && fat_name[i] <= 'z')
            return -1;
        if (fat_name[i] == '/')
            return -1;
        if (fat_name[i] == '\"')
            return -1;
        if (fat_name[i] == '*')
            return -1;
        if (fat_name[i] == ',')
            return -1;
        if (fat_name[i] == '.')
            return -1;
        if (fat_name[i] == '?')
            return -1;
        if (fat_name[i] == '[')
            return -1;
        if (fat_name[i] == ']')
            return -1;
        if (fat_name[i] == '|')
            return -1;

        dst[offset++] = fat_name[i];
    }
    if (offset == 0)
        return -1;
    const char* ext = entry->ext;
    if (ext[0] == ' ' && ext[1] == ' ' && ext[2] == ' ') {  // no ext name
        dst[offset] = '\0';
        return;
    }
    dst[offset++] = '.';
    for (int i = 0; i < 3; i++) {
        if (ext[i] == ' ')
            break;
        dst[offset++] = ext[i];
    }
    dst[offset] = '\0';
    return 0;
}

/**
 * Parse the directory file from external storage
*/
static int parse_dir_entries(FS_VNODE* node) {

    FAT32_FS_INTERNAL* mount_internal = (FAT32_FS_INTERNAL*)node->mount->internal;
    FAT32_VNODE_INTERNAL* node_internal = (FAT32_VNODE_INTERNAL*)node->internal;

    // entry is 32 bytes it is ok don't using unalign
    UPTR* sector_buf = kmalloc(mount_internal->bytes_per_sector);
    U32 index = node_internal->cluster_id;

    while (TRUE) {
        if (index < 2 || index >= FAT32_FAT_BAD)
            break;
        node->mount->read((mount_internal->data_start_sector + index) * mount_internal->bytes_per_sector, sector_buf, mount_internal->bytes_per_sector);
        index = mount_internal->fat_table[index];
        NS_DPRINT("[FS] FAT32fs: parse_dir_entries: next index: %u\n", index);

        FAT32_DIR_ENTRY* entry = (FAT32_DIR_ENTRY*) sector_buf;
        while (entry < (sector_buf + mount_internal->bytes_per_sector)) {
            
            // https://averstak.tripod.com/fatdox/dir.htm#atr
            // If Attribute is equal to 0Fh (Read Only, Hidden, System, Volume Label) then this entry does not contain the alias, but it is used as part of the long filename or long directory name.
            if (entry->attribute == FAT32_DIR_ENTRY_ATTR_LONG_NAME) {  // ignore LFN
                entry++;
                continue;
            }

            if (entry->name[0] == FAT32_DIR_ENTRY_UNUSED || entry->name[0] == ' ') {
                entry++;
                continue;
            }
            if (entry->name[0] == FAT32_DIR_ENTRY_LAST_AND_UNUSED) {
                NS_DPRINT("[FS] FAT32fs: parse_dir_entries: getting unused break.\n");
                index = 0;                                                      // let the loop end
                break;
            }

            U32 cluster_id = (utils_read_unaligned_u16(&entry->high_block_index) << 16) + utils_read_unaligned_u16(&entry->low_block_index);

            // corrupt file
            if (cluster_id <= ((FAT32_VNODE_INTERNAL*)node->mount->root->internal)->cluster_id) {
                NS_DPRINT("[FS][ERROR] FAT32fs: parse_dir_entries(): Corrupt file cluster index.\n");
                entry++;
                continue;
            }

            char tmp_name[13];
            if (convert_fat_name(entry, tmp_name) == -1) { // corrupt file name
                entry++;
                continue;
            }
            NS_DPRINT("[FS] FAT32fs: file name: %s\n", tmp_name);

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
            new_internal->cluster_id = cluster_id;
            NS_DPRINT("[FS] FAT32fs: block id: %u\n", new_internal->cluster_id);
            new_internal->is_dirty = FALSE;
            new_internal->is_loaded = FALSE;    // not loaded to faster the searching when lookup
            
            new_node->content_size = entry->file_size;
            NS_DPRINT("[FS] FAT32fs: file size: %u\n", new_node->content_size);

            // common modification
            new_node->mount = node->mount;
            new_node->v_ops = node->v_ops;
            new_node->f_ops = node->f_ops;
            new_node->parent = node;

            link_list_push_back(&node->childs, &new_node->self);
            node->child_num++;

            entry++;
        }
        
    }

    return 0;
}