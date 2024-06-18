#include "fat32.h"

#include "mem.h"
#include "string.h"
#include "uart1.h"
#include "utli.h"
#include "vm.h"

static filesystem* fat32fs = (filesystem*)0;
static vnode_operations* fat32fs_v_ops = (vnode_operations*)0;
static file_operations* fat32fs_f_ops = (file_operations*)0;
static fat32_metadata* fat32_md = (fat32_metadata*)0;
static fat32_cache_block* fat32_cache_list = (fat32_cache_block*)0;

static void cache_list_push_front(uint32_t block_idx, void* buf,
                                  uint8_t dirty_flag) {
  fat32_cache_block* node =
      (fat32_cache_block*)malloc(sizeof(fat32_cache_block));
  node->block_idx = block_idx;
  memcpy((void*)node->block, buf, BLOCK_SIZE);
  node->dirty_flag = dirty_flag;
  node->next = fat32_cache_list;
  fat32_cache_list = node;
}

static fat32_cache_block* cache_list_find(uint32_t block_idx) {
  for (fat32_cache_block* ptr = fat32_cache_list; ptr != (fat32_cache_block*)0;
       ptr = ptr->next) {
    if (ptr->block_idx == block_idx) {
      return ptr;
    }
  }
  return (fat32_cache_block*)0;
}

static int32_t fat32fs_syncfs() {
  fat32_cache_block* node = fat32_cache_list;
  while (node) {
    fat32_cache_block* next = node->next;
    if (node->dirty_flag) {
      sd_writeblock(node->block_idx, (void*)node->block);
    }
    free((void*)node);
    node = next;
  }
  fat32_cache_list = (fat32_cache_block*)0;
  return 0;
}

static void readblock(uint32_t block_idx, void* buf) {
  fat32_cache_block* cache_node = cache_list_find(block_idx);
  if (cache_node) {
    memcpy(buf, (void*)cache_node->block, BLOCK_SIZE);
    return;
  }
  sd_readblock(block_idx, buf);
  cache_list_push_front(block_idx, buf, 0);
}

static void writeblock(uint32_t block_idx, void* buf) {
  fat32_cache_block* cache_node = cache_list_find(block_idx);
  if (cache_node) {
    cache_node->dirty_flag = 1;
    memcpy((void*)cache_node->block, buf, BLOCK_SIZE);
  } else {
    cache_list_push_front(block_idx, buf, 1);
    // sd_writeblock(block_idx, buf);
  }
}

static uint32_t getFirstCluster(fat32_dirent_sfn* dirent) {
  return ((uint32_t)dirent->firstClusterHigh << 16) | dirent->firstClusterLow;
}

static uint32_t clusteridx_2_datablockidx(uint32_t cluster_idx) {
  return fat32_md->data_region_block_idx +
         (cluster_idx - fat32_md->root_dir_cluster_idx) *
             fat32_md->n_sectors_per_cluster;
}

static uint32_t clusteridx_2_fatblockidx(uint32_t cluster_idx) {
  return fat32_md->fat_region_block_idx + (cluster_idx / N_ENTRY_PER_FAT);
}

static void fill_SFN_fname(fat32_dirent_sfn* dirent, const char* filename) {
  uint32_t n = strlen(filename);
  uint32_t dot_idx = n;
  for (uint32_t i = 0; i < n; i++) {
    if (filename[i] == '.') {
      dot_idx = i;
      break;
    }
  }
  for (uint32_t i = 0; i < 8; i++) {
    if (i < dot_idx) {
      dirent->name[i] = filename[i];
    } else {
      dirent->name[i] = ' ';
    }
  }
  for (uint32_t i = 0; i < 3; i++) {
    if (dot_idx + i + 1 < n) {
      dirent->ext[i] = filename[dot_idx + i + 1];
    } else {
      dirent->ext[i] = ' ';
    }
  }
}

static void get_SFN_fname(fat32_dirent_sfn* dirent, char* filename) {
  uint32_t idx = 0;
  for (uint32_t i = 0; i < 8 && dirent->name[i] != ' '; i++) {
    filename[idx++] = dirent->name[i];
  }
  filename[idx++] = '.';
  for (uint32_t i = 0; i < 3 && dirent->ext[i] != ' '; i++) {
    filename[idx++] = dirent->ext[i];
  }
  filename[idx++] = '\0';
}

static uint32_t getFreeFatEntry() {
  uint32_t fat_buf[N_ENTRY_PER_FAT];
  int32_t found_cluster_idx = -1;
  for (uint32_t i = 0; found_cluster_idx == -1; i += N_ENTRY_PER_FAT) {
    readblock(clusteridx_2_fatblockidx(i), (void*)fat_buf);
    for (uint32_t j = 0; j < N_ENTRY_PER_FAT; j++) {
      if (fat_buf[j] == FREE_CLUSTER) {
        fat_buf[j] = EOC;
        writeblock(clusteridx_2_fatblockidx(i), (void*)fat_buf);
        found_cluster_idx = i + j;
        break;
      }
    }
  }
  return found_cluster_idx;
}

static file* fat32fs_create_fd(vnode* file_node) {
  file* fp = (file*)malloc(sizeof(file));
  if (!fp) {
    return (file*)0;
  }
  fp->vnode = file_node;
  fp->f_ops = file_node->f_ops;
  fp->f_pos = 0;
  return fp;
}

static vnode* fat32fs_create_vnode(vnode* parent, const char* name,
                                   uint8_t type, uint32_t dirent_cluster,
                                   uint32_t first_cluster, uint32_t size) {
  vnode* fat32fs_vnode = (vnode*)malloc(sizeof(vnode));
  if (!fat32fs_vnode) {
    return (vnode*)0;
  }
  strcpy(fat32fs_vnode->name, name);
  fat32fs_vnode->parent = parent;
  if (parent) {
    for (uint32_t i = 0; i < MAX_SUBDIR_N; i++) {
      if (parent->subdirs[i] == (vnode*)0) {
        parent->subdirs[i] = fat32fs_vnode;
        break;
      }
    }
  }
  for (uint32_t i = 0; i < MAX_SUBDIR_N; i++) {
    fat32fs_vnode->subdirs[i] = (vnode*)0;
  }
  fat32fs_vnode->mountpoint = (mount*)0;
  fat32fs_vnode->v_ops = fat32fs_v_ops;
  fat32fs_vnode->f_ops = fat32fs_f_ops;
  fat32fs_vnode->type = type;

  fat32_internal* child_internal =
      (fat32_internal*)malloc(sizeof(fat32_internal));
  child_internal->dirent_cluster = dirent_cluster;
  child_internal->first_cluster = first_cluster;
  child_internal->size = size;
  fat32fs_vnode->internal = (void*)child_internal;

  return fat32fs_vnode;
}

static int32_t fat32fs_vnode_lookup(vnode* dir_node, vnode** target,
                                    const char* component_name) {
  for (uint32_t i = 0; i < MAX_SUBDIR_N; i++) {
    if (dir_node->subdirs[i] &&
        !strcmp(dir_node->subdirs[i]->name, component_name)) {
      *target = dir_node->subdirs[i];
      return 0;
    }
  }
  return -1;
}

static int32_t fat32fs_vnode_create(vnode* dir_node, vnode** target,
                                    const char* component_name) {
  fat32_internal* dir_internal = (fat32_internal*)dir_node->internal;
  uint32_t block_idx = clusteridx_2_datablockidx(dir_internal->first_cluster);
  uint8_t buf[BLOCK_SIZE];
  fat32_dirent_sfn* dirent = (fat32_dirent_sfn*)0;

  readblock(block_idx, (void*)buf);
  for (uint32_t i = 0; i < SECTOR_SIZE; i += DIRENT_SIZE) {
    if (buf[i] == 0x00 || buf[i] == 0xE5) {
      dirent = (fat32_dirent_sfn*)&buf[i];
      fill_SFN_fname(dirent, component_name);
      dirent->attr = SFN_ATTR;
      dirent->rsv = 0x0;
      dirent->crtTimeTenth = 0x80;
      dirent->crtTime = 0x8CC;
      dirent->crtDate = 0x58D5;
      dirent->lastAcccessDate = 0x58D5;
      dirent->wrtTime = 0x1A44;
      dirent->wrtDate = 0x58D4;
      dirent->fileSize = 0;
      dirent->firstClusterHigh = dirent->firstClusterLow = 0;
      uint32_t found_cluster_idx = getFreeFatEntry();

      uart_send_string("found_cluster_idx: ");
      uart_int(found_cluster_idx);
      uart_send_string("\r\n");

      dirent->firstClusterHigh = ((found_cluster_idx & 0xFFFF0000) >> 16);
      dirent->firstClusterLow = (found_cluster_idx & 0x0000FFFF);
      writeblock(block_idx, (void*)buf);
      break;
    }
  }
  if (!dirent) {
    return -1;
  }

  *target = fat32fs_create_vnode(dir_node, component_name, REGULAR_FILE,
                                 dir_internal->first_cluster,
                                 getFirstCluster(dirent), dirent->fileSize);
  if (!*target) {
    return -1;
  }
  return 0;
}

static int32_t fat32fs_vnode_mkdir(vnode* dir_node, vnode** target,
                                   const char* component_name) {
  if (dir_node->type != DIRECTORY) {
    uart_puts("fat32fs_vnode_mkdir: target_node isn't a directory");
    return -1;
  }

  fat32_internal* dir_internal = (fat32_internal*)dir_node->internal;
  *target = fat32fs_create_vnode(dir_node, component_name, DIRECTORY,
                                 dir_internal->first_cluster, -1, 0);
  if (!*target) {
    uart_puts("fat32fs_vnode_mkdir: fat32fs_create_vnode fails");
    return -1;
  }
  return 0;
}

static int32_t fat32fs_file_open(vnode* file_node, file** fp) {
  *fp = fat32fs_create_fd(file_node);
  if (!(*fp)) {
    return -1;
  }
  return 0;
}

static int32_t fat32fs_file_close(file* fp) {
  free((void*)fp);
  return 0;
}

static int32_t fat32fs_file_read(file* fp, void* buf, uint32_t len) {
  uint32_t fat_buf[N_ENTRY_PER_FAT];
  fat32_internal* internal = (fat32_internal*)fp->vnode->internal;
  uint32_t cluster_idx = internal->first_cluster;
  for (uint32_t cnt = 0; cnt + BLOCK_SIZE < fp->f_pos; cnt += BLOCK_SIZE) {
    readblock(clusteridx_2_fatblockidx(cluster_idx), (void*)fat_buf);
    cluster_idx = fat_buf[cluster_idx % N_ENTRY_PER_FAT];
  }

  uint8_t ker_buf[BLOCK_SIZE];
  uint32_t remain_len = len;
  uint32_t ori_pos = fp->f_pos;
  while (remain_len > 0 && fp->f_pos < internal->size && cluster_idx != EOC) {
    readblock(clusteridx_2_datablockidx(cluster_idx), ker_buf);
    uint32_t shift = (remain_len < BLOCK_SIZE) ? remain_len : BLOCK_SIZE;
    shift = (fp->f_pos + shift < internal->size) ? shift
                                                 : (internal->size - fp->f_pos);
    memcpy((void*)buf + (fp->f_pos - ori_pos),
           (void*)ker_buf + (fp->f_pos % BLOCK_SIZE), shift);
    fp->f_pos += shift;
    remain_len -= shift;

    if (remain_len > 0 && fp->f_pos < internal->size) {
      readblock(clusteridx_2_fatblockidx(cluster_idx), (void*)fat_buf);
      cluster_idx = fat_buf[cluster_idx % N_ENTRY_PER_FAT];
    }
  }
  return fp->f_pos - ori_pos;
}

static int32_t fat32fs_file_write(file* fp, const void* buf, uint32_t len) {
  uint32_t fat_buf[N_ENTRY_PER_FAT];
  fat32_internal* internal = (fat32_internal*)fp->vnode->internal;
  uint32_t cluster_idx = internal->first_cluster;
  for (uint32_t cnt = 0; cnt + BLOCK_SIZE < fp->f_pos; cnt += BLOCK_SIZE) {
    readblock(clusteridx_2_fatblockidx(cluster_idx), (void*)fat_buf);
    cluster_idx = fat_buf[cluster_idx % N_ENTRY_PER_FAT];
  }

  uint8_t ker_buf[BLOCK_SIZE];
  uint32_t ori_pos = fp->f_pos;
  uint32_t remain_len = len;
  void* src = (void*)buf;

  while (remain_len > 0) {
    uint32_t shift = (remain_len < BLOCK_SIZE) ? remain_len : BLOCK_SIZE;
    memcpy((void*)ker_buf, src + (fp->f_pos - ori_pos), shift);
    for (uint32_t i = shift; i < BLOCK_SIZE; i++) {
      ker_buf[i] = 0x0;
    }
    fp->f_pos += shift;
    remain_len -= shift;
    writeblock(clusteridx_2_datablockidx(cluster_idx), (void*)ker_buf);

    if (remain_len > 0) {
      readblock(clusteridx_2_fatblockidx(cluster_idx), (void*)fat_buf);
      if (fat_buf[cluster_idx % N_ENTRY_PER_FAT] == EOC) {
        fat_buf[cluster_idx % N_ENTRY_PER_FAT] = getFreeFatEntry();
      }
      cluster_idx = fat_buf[cluster_idx % N_ENTRY_PER_FAT];
    }
  }

  // set new size in internal and its dirent
  if (fp->f_pos > internal->size) {
    internal->size = fp->f_pos;
    uint32_t dirBlockIdx = clusteridx_2_datablockidx(internal->dirent_cluster);
    uint8_t end = 0;
    char filename[15];
    while (!end) {
      readblock(dirBlockIdx, (void*)ker_buf);
      for (uint32_t i = 0; i < SECTOR_SIZE; i += DIRENT_SIZE) {
        if (ker_buf[i] == 0x00) {
          end = 1;
          break;
        }
        if (ker_buf[i] == 0xE5) {  // deletd file
          continue;
        }
        fat32_dirent_sfn* dirent = (fat32_dirent_sfn*)&ker_buf[i];

        if (dirent->attr == SFN_ATTR) {
          uart_int(dirent->fileSize);
          uart_send_string(" ");
          uart_int(clusteridx_2_datablockidx(getFirstCluster(dirent)));
          uart_send_string(" ");
          uint32_t fat_buf[N_ENTRY_PER_FAT];
          sd_readblock(clusteridx_2_fatblockidx(getFirstCluster(dirent)),
                       (void*)fat_buf);
          uart_hex(fat_buf[getFirstCluster(dirent) % N_ENTRY_PER_FAT]);
          uart_send_string(" ");
          get_SFN_fname(dirent, filename);
          uart_puts(filename);
          if (!strcmp(filename, fp->vnode->name)) {
            dirent->fileSize = fp->f_pos;
            writeblock(dirBlockIdx, (void*)ker_buf);
          }
        }
      }
      dirBlockIdx++;
    }
  }

  return fp->f_pos - ori_pos;
}

static int32_t fat32fs_setup_mount(filesystem* fs, mount* mount,
                                   const char* name) {
  mount->fs = fs;
  mount->root = fat32fs_create_vnode((vnode*)0, name, DIRECTORY, -1,
                                     fat32_md->root_dir_cluster_idx, 0);
  if (!mount->root) {
    return -1;
  }
  mount->root->mountpoint = mount;

  char filename[15];
  uint8_t buf[BLOCK_SIZE];
  fat32_internal* dir_internal = (fat32_internal*)mount->root->internal;

  // uint32_t fat_buf[N_ENTRY_PER_FAT];
  // uint32_t cluster_idx = dir_internal->first_cluster;
  // while (cluster_idx != EOC) {
  //   uart_hex(cluster_idx);
  //   uart_send_string(" ");
  //   readblock(clusteridx_2_fatblockidx(cluster_idx), (void*)fat_buf);
  //   wait_usec(1000000);
  //   cluster_idx = fat_buf[cluster_idx % N_ENTRY_PER_FAT];
  // }
  // uart_hex(cluster_idx);
  // uart_send_string("\r\n");

  uint32_t block_idx = clusteridx_2_datablockidx(dir_internal->first_cluster);

  readblock(block_idx, (void*)buf);
  for (uint32_t i = 0; i < SECTOR_SIZE; i += DIRENT_SIZE) {
    if (buf[i] == 0x00) {
      break;
    }
    if (buf[i] == 0xE5) {  // deletd file
      continue;
    }
    fat32_dirent_sfn* dirent = (fat32_dirent_sfn*)&buf[i];

    if (dirent->attr == SFN_ATTR) {
      uart_int(i / DIRENT_SIZE);
      uart_send_string(": ");
      uart_int(dirent->fileSize);
      uart_send_string(" ");
      uart_int(clusteridx_2_datablockidx(getFirstCluster(dirent)));
      uart_send_string(" ");
      uint32_t fat_buf[N_ENTRY_PER_FAT];
      sd_readblock(clusteridx_2_fatblockidx(getFirstCluster(dirent)),
                   (void*)fat_buf);
      uart_hex(fat_buf[getFirstCluster(dirent) % N_ENTRY_PER_FAT]);
      uart_send_string(" ");
      get_SFN_fname(dirent, filename);
      uart_puts(filename);
      // if (!strcmp(filename, "FAT_R.TXT")) {
      //   uart_hex(dirent->rsv);
      //   uart_send_string(" ");
      //   uart_hex(dirent->crtTimeTenth);
      //   uart_send_string(" ");
      //   uart_hex(dirent->crtTime);
      //   uart_send_string(" ");
      //   uart_hex(dirent->crtDate);
      //   uart_send_string(" ");
      //   uart_hex(dirent->lastAcccessDate);
      //   uart_send_string(" ");
      //   uart_hex(dirent->wrtTime);
      //   uart_send_string(" ");
      //   uart_hex(dirent->wrtDate);
      //   uart_send_string("\r\n");
      // }
      fat32fs_create_vnode(mount->root, filename, REGULAR_FILE,
                           dir_internal->first_cluster, getFirstCluster(dirent),
                           dirent->fileSize);
    }
  }

  return 0;
}

static int32_t fat32fs_register_fs() {
  if (!fat32fs_v_ops) {
    fat32fs_v_ops = (vnode_operations*)malloc(sizeof(vnode_operations));
    if (!fat32fs_v_ops) {
      return -1;
    }
    fat32fs_v_ops->create =
        (vfs_create_fp_t)phy2vir((uint64_t)fat32fs_vnode_create);
    fat32fs_v_ops->lookup =
        (vfs_lookup_fp_t)phy2vir((uint64_t)fat32fs_vnode_lookup);
    fat32fs_v_ops->mkdir =
        (vfs_mkdir_fp_t)phy2vir((uint64_t)fat32fs_vnode_mkdir);
  }

  if (!fat32fs_f_ops) {
    fat32fs_f_ops = (file_operations*)malloc(sizeof(file_operations));
    if (!fat32fs_f_ops) {
      return -1;
    }
    fat32fs_f_ops->open = (vfs_open_fp_t)phy2vir((uint64_t)fat32fs_file_open);
    fat32fs_f_ops->close =
        (vfs_close_fp_t)phy2vir((uint64_t)fat32fs_file_close);
    fat32fs_f_ops->read = (vfs_read_fp_t)phy2vir((uint64_t)fat32fs_file_read);
    fat32fs_f_ops->write =
        (vfs_write_fp_t)phy2vir((uint64_t)fat32fs_file_write);
  }
  return 0;
}

filesystem* fat32fs_init() {
  if (!fat32fs) {
    fat32fs = (filesystem*)malloc(sizeof(filesystem));
    strcpy(fat32fs->name, "fat32fs");
    fat32fs->setup_mnt =
        (vfs_setup_mount)phy2vir((uint64_t)fat32fs_setup_mount);
    fat32fs->register_fs =
        (vfs_register_fs)phy2vir((uint64_t)fat32fs_register_fs);
    fat32fs->syncfs = (vfs_syncfs)phy2vir((uint64_t)fat32fs_syncfs);
  }
  return fat32fs;
}

void fat32fs_init_metadata(mbr_partition_entry* partition_entry,
                           fat32_bootsector* fat32_bootsec) {
  if (!fat32_md) {
    fat32_md = (fat32_metadata*)malloc(sizeof(fat32_metadata));
    fat32_md->fat_region_block_idx =
        partition_entry->first_sector_lba + fat32_bootsec->n_reserved_sectors;
    fat32_md->data_region_block_idx =
        partition_entry->first_sector_lba + fat32_bootsec->n_reserved_sectors +
        fat32_bootsec->n_sectors_per_fat * fat32_bootsec->n_fats;
    fat32_md->n_fats = fat32_bootsec->n_fats;
    fat32_md->n_sectors = fat32_bootsec->n_sectors;
    fat32_md->n_sectors_per_cluster = fat32_bootsec->n_sectors_per_cluster;
    fat32_md->n_sectors_per_fat = fat32_bootsec->n_sectors_per_fat;
    fat32_md->root_dir_cluster_idx = fat32_bootsec->root_dir_cluster_idx;

    uart_send_string("fat32_md->fat_region_block_idx: ");
    uart_int(fat32_md->fat_region_block_idx);
    uart_send_string("\r\n");
    uart_send_string("fat32_md->data_region_block_idx: ");
    uart_int(fat32_md->data_region_block_idx);
    uart_send_string("\r\n");
    uart_send_string("fat32_md->n_fats: ");
    uart_int(fat32_md->n_fats);
    uart_send_string("\r\n");
    uart_send_string("fat32_md->n_sectors: ");
    uart_int(fat32_md->n_sectors);
    uart_send_string("\r\n");
    uart_send_string("fat32_md->n_sectors_per_cluster: ");
    uart_int(fat32_md->n_sectors_per_cluster);
    uart_send_string("\r\n");
    uart_send_string("fat32_md->n_sectors_per_fat: ");
    uart_int(fat32_md->n_sectors_per_fat);
    uart_send_string("\r\n");
    uart_send_string("fat32_md->root_dir_cluster_idx: ");
    uart_int(fat32_md->root_dir_cluster_idx);
    uart_send_string("\r\n");
  }
}