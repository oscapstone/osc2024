#include "vfs.h"
#include "alloc.h"
#include "string.h"
#include "tmpfs.h"
#include "errno.h"
#include "tmpfs.h"
#include "io.h"
#include "schedule.h"

extern struct task_struct* current;

struct mount* rootfs;
struct filesystem global_fs[MAX_FILESYSTEM];
// struct file_descriptor_table global_fd_table;


static struct vnode* next_step(struct vnode* start_node, const char* pathname, struct vnode** target_dir);
static struct file* create_file(struct vnode* vnode, int flags);
static char* get_file_name(const char* pathname);
static void simplify_path(char* pathname);

// void global_fd_table_init()
// {
//     for(int i=0; i<MAX_OPEN_FILE; i++)
//     {
//         global_fd_table.files[i] = NULL;
//     }
// }


void rootfs_init()
{
    // pre init
    // global_fd_table_init();
    // 1. create a tmpfs filesystem
    // 2. create a rootfs mount
    // 3. setup rootfs mount

    struct filesystem* tmpfs = (struct filesystem*)dynamic_alloc(sizeof(struct filesystem));
    tmpfs->name = (char*)dynamic_alloc(16);
    strcpy(tmpfs->name, "tmpfs");
    
    tmpfs->setup_mount = tmpfs_setup_mount;
    register_filesystem(tmpfs);
    
    rootfs = (struct mount*)dynamic_alloc(sizeof(struct mount));
    tmpfs->setup_mount(tmpfs, rootfs);
}


int register_filesystem(struct filesystem* fs) { // ensure there is sufficient memory for the filesystem
  // register the file system to the kernel.
  // you can also initialize memory pool of the file system here.
  if(!strcmp(fs->name, "tmpfs"))
  {
      //initialize memory pool of the file system
      return tmpfs_register();
  }
  // [TODO]
  // else if(!strcmp(fs->name, "initramfs"))
  // {
  //   return initramfs_register();
  // }
  return -1;
}

int vfs_open(const char* pathname, int flags, struct file** target) {
  // 1. Lookup pathname
  // 2. Create a new file handle for this vnode if found.
  // 3. Create a new file if O_CREAT is specified in flags and vnode not found
  // lookup error code shows if file exist or not or other error occurs
  // 4. Return error code if fails

  struct vnode* vnode;
  int ret = vfs_lookup(pathname, &vnode);
  if(ret != 0 && flags != O_CREAT){
    printf("\r\n[ERROR] Cannot find the file");
    return -1;
  }
  else if(ret == 0 && flags == O_CREAT){
    printf("\r\n[ERROR] File already exists");
    return -1;
  }
  else if(flags == O_CREAT){
    struct vnode* target_dir; 
    vfs_get_node(pathname, &target_dir);
    if(target_dir == NULL){
      printf("\r\n[ERROR] Cannot find the parent directory");
      return -1;
    }

    char* file_name = get_file_name(pathname);

    ret = target_dir->v_ops->create(target_dir, &vnode, file_name);
   
    *target = create_file(vnode, flags);

    if(ret != 0){
      printf("\r\n[ERROR] Cannot create the file");
      return -1;
    }
    return ret;
  }

  struct file* file = create_file(vnode, flags);
  if(vnode->f_ops->open(vnode, &file) != 0){
    printf("\r\n[ERROR] Cannot open the file");
    return -1;
  }

  *target = file;

  return 0;
}

int vfs_close(struct file* file) {
  // 1. release the file handle
  // 2. Return error code if fails
  int ret = file->f_ops->close(file);
  return ret;
}

int vfs_write(struct file* file, const void* buf, size_t len) {
  // 1. write len byte from buf to the opened file.
  // 2. return written size or error code if an error occurs.
  int ret = file->f_ops->write(file, buf, len);
  return ret;
}

int vfs_read(struct file* file, void* buf, size_t len) {
  // 1. read min(len, readable size) byte to buf from the opened file.
  // 2. block if nothing to read for FIFO type
  // 2. return read size or error code if an error occurs.
  int ret = file->f_ops->read(file, buf, len);
  return ret;
}

int vfs_mkdir(const char* pathname)
{

    struct vnode* target_dir;
    struct vnode* vnode = vfs_get_node(pathname, &target_dir);
    if(vnode != NULL)
    {
        printf("\r\n[ERROR] Directory already exists");
        return CERROR;
    }
    
    if(target_dir == NULL)
    {
      printf("\r\n[ERROR] Cannot find the parent directory");
      return CERROR;
    }
    struct vnode* new_dir;
    char* file_name = get_file_name(pathname);
    printf("\r\n[MKDIR] pathname: "); printf(pathname); printf(" , filename: "); printf(file_name);

    int ret = target_dir->v_ops->mkdir(target_dir, &new_dir, file_name);
    if(ret != 0)
    {
        printf("\r\n[ERROR] Cannot create directory");
        dfree(file_name);
        return CERROR;
    }
    dfree(file_name);
    return 0;
}

int vfs_mount(const char* target, const char* filesystem) // mount the filesystem to the target
{
    struct mount* mount_point = (struct mount*)dynamic_alloc(sizeof(struct mount));
    struct filesystem* fs = (struct filesystem*)dynamic_alloc(sizeof(struct filesystem));

    // int ret = vfs_lookup(target, NULL);
    // if(ret == 0)
    // {
    //   printf("[ERROR] Target already exists\n");      
    //   return CERROR;
    // }

    struct vnode* target_dir; 
    vfs_get_node(target, &target_dir);
    if(target_dir == NULL)
    {
      printf("\r\n[ERROR] Cannot find the parent directory");
      return CERROR;
    }

    fs->name = (char*)dynamic_alloc(16);
    if(!strcmp(filesystem, "tmpfs"))
    {
      
      strcpy(fs->name, "tmpfs");

      fs->setup_mount = tmpfs_setup_mount;
      
    }
    else if(!strcmp(filesystem, "initramfs"))
    {
      strcpy(fs->name, "tmpfs");

      // fs->setup_mount = tmpfs_setup_mount;
    }
    else
    {
      printf("\r\n[ERROR] Filesystem not found");
      return CERROR;
    }

    register_filesystem(fs);
    fs->setup_mount(fs, mount_point);

    target_dir->mount = mount_point;
    mount_point->root->parent = target_dir->parent;

    return 0;
}
int vfs_lookup(const char* pathname, struct vnode** target)
{
    struct vnode* vnode = vfs_get_node(pathname, NULL);
    if(vnode == NULL)
    {
        return -1;
    }
    *target = vnode;
    return 0;
}

int vfs_list(const char* pathname)
{ 
    struct vnode* vnode;
    int ret = vfs_lookup(pathname, &vnode);
    
    if(ret != 0)
    {
        printf("\r\n[ERROR] Cannot find the directory");
        return -1;
    }
    
    ret = vnode->v_ops->list(vnode);
    if(ret != 0)
    {
        printf("\r\n[ERROR] Cannot list the directory");
        return CERROR;
    }
    return 0;
}


int vfs_chdir(const char* relative_path)
{
    char* current_abs_path = current->cwd;
    char* new_abs_path = (char*)dynamic_alloc(strlen(current_abs_path) + strlen(relative_path) + 4);
    strcpy(new_abs_path, current_abs_path);
    if(relative_path[0] == '/'){ // absolute path
      strcpy(new_abs_path, relative_path);
    }
    else{ // relative path
      strcpy(new_abs_path, current_abs_path);
      strcpy(new_abs_path + strlen(current_abs_path), relative_path);
    }

    if(new_abs_path[strlen(new_abs_path)-1] != '/'){ // add '/' at the end
      new_abs_path[strlen(new_abs_path)] = '/';
      new_abs_path[strlen(new_abs_path)+1] = '\0';
    }

    // printf("\r\n[CHDIR] new_abs_path: "); printf(new_abs_path);
    struct vnode* vnode = vfs_get_node(new_abs_path, NULL);

    if(vnode == NULL)
    {
        printf("\r\n[ERROR] Cannot find the directory");
        return CERROR;
    }

    simplify_path(new_abs_path);
    printf("\r\n[CHDIR] new_abs_path(simplified): "); printf(new_abs_path);

    preempt_enable();
    strcpy(current->cwd, new_abs_path);
    preempt_disable();

    return 0;
}

struct vnode* vfs_get_node(const char* pathname, struct vnode** target_dir)
{
  struct vnode* start_node;
  
  if(pathname[0] == '/'){
    start_node = rootfs->root;
    return next_step(start_node, pathname+1, target_dir);
  }
  else{ // [TODO] relative path
    start_node = vfs_get_node(current->cwd, NULL);
    return next_step(start_node, pathname, target_dir);
  }
  return NULL;
}

static struct vnode* next_step(struct vnode* start_node, const char* pathname, struct vnode** target_dir)
{
  char* tmp_path = (char*)dynamic_alloc(strlen(pathname)+1);
  strcpy(tmp_path, pathname);
  char* token = strtok(tmp_path, "/");
  // if(strlen(token) == 0) { token = NULL; }
  struct vnode* current_node = start_node;

  while(token != NULL){
    *target_dir = current_node;
    if(!strcmp(token, "..")){
      if(current_node->parent == NULL){
        current_node = NULL;
        goto next_step_end;
      }
      current_node = current_node->parent;
    }
    else if(!strcmp(token, ".")){
      // do nothing
    }
    else{
      struct vnode* next_node;
      if(current_node->v_ops->lookup(current_node, &next_node, token) != 0){
        current_node = NULL;
        goto next_step_end;
      }
      current_node = next_node;
    }
    // printf("\r\nsame: "); printf_int(current_node->parent == rootfs->root);
    token = strtok(NULL, "/");
  }
next_step_end:
  dfree(tmp_path);
  return current_node;
}

static struct file* create_file(struct vnode* vnode, int flags)
{
  struct file* file = (struct file*)dynamic_alloc(sizeof(struct file));
  file->vnode = vnode;
  file->f_pos = 0;
  file->f_ops = vnode->f_ops;
  file->flags = flags;
  return file;
}


static char* get_file_name(const char* pathname)
{
  char* file_name = (char*)dynamic_alloc(16);
    int i;
    for(i = strlen(pathname)-1; i >= 0; i--){
      if(pathname[i] == '/'){
        break;
      }
    }
    if(i == -1){
      strcpy(file_name, pathname);
    }
    else{ 
      strcpy(file_name, pathname+i+1); 
    }
    return file_name;
}

static void simplify_path(char* pathname)
{
  
  printf("\r\n"); printf(__func__);
  #define MAX_SEGMENTS 256
  
  char* tmp_path = (char*)dynamic_alloc(strlen(pathname)+1);
   char* new_path = (char*)dynamic_alloc(strlen(pathname)+1);

  strcpy(tmp_path, pathname);
  
  char* segments[MAX_SEGMENTS];
  int n_segments = 0;

  char* token = strtok(tmp_path, "/");
  int isFirst = 1;  // avoid the first NULL token
  while(token != NULL || isFirst == 1){
    isFirst = 0;
    if(token != NULL){ 
      segments[n_segments] = (char*)dynamic_alloc(strlen(token)+1);
      // printf("\r\nsegment at: "); printf_hex(segments[n_segments]); printf(" , "); printf_int(n_segments);
      strcpy(segments[n_segments++], token);
    }
    token = strtok(NULL, "/");
  }

  // printf("\r\n"); printf(segments[0]); printf_hex((void*)segments[0]);

  // for(int i=0; i<n_segments; i++){
  //   printf("\r\n[DEBUG] segments: "); printf(segments[i]);
  // }

  int need_segment[MAX_SEGMENTS];
  int need_idx = 0;
  for(int i=0; i<n_segments; i++){
    if(segments[i] == NULL || !strcmp(segments[i], ".")) continue;
    if(!strcmp(segments[i], "..")){
      if(need_idx > 0){
        need_idx--;
      }
    }
    else{
      need_segment[need_idx++] = i;
    }
  }

  // printf("\r\n[DEBUG] need_idx: "); printf_int(need_idx);

  for(int i=0; i<need_idx; i++){ // concatenate the segments
    if(i == 0){
      strcpy(new_path, "/\0");
    }
    strcpy(new_path + strlen(new_path), segments[need_segment[i]]);
    strcpy(new_path + strlen(new_path), "/");
    // printf("\r\n[DEBUG] new_path: "); printf(new_path);
    dfree(segments[need_segment[i]]);
  }

  if(need_idx == 0){
    strcpy(new_path, "/\0");
  }

  if(new_path[strlen(new_path)-1] != '/')
    strcpy(new_path + strlen(new_path), "/\0");
  else
    new_path[strlen(new_path)] = '\0';
  // printf("\r\n[DEBUG] new_path: "); printf(new_path);
  strcpy(pathname, new_path);
  dfree(tmp_path);
  dfree(new_path);
  // return pathname;
}