#include "tmpfs.h"
#include "vfs.h"
#include "string.h"
#include "memory.h"
#include "uart1.h"

struct file_operations tmpfs_file_operations = {tmpfs_write,tmpfs_read,tmpfs_open,tmpfs_close,tmpfs_lseek64,tmpfs_getsize};
struct vnode_operations tmpfs_vnode_operations = {tmpfs_lookup,tmpfs_create,tmpfs_mkdir};

// Basic 1
// 由於每個file system都有自己的初始化方法，因此VFS應該為每個file system提供註冊的介面
// 然後，users可以透過指定名稱來mount file system
int register_tmpfs(){
    struct filesystem fs;
    // 創一個filesystem structure "tmpfs"
    fs.name = "tmpfs";
    // 將"tmpfs" setup_mount設為`tmpfs_setup_mount` function
    fs.setup_mount = tmpfs_setup_mount;
    // 通過register_filesystem function將其註冊到VFS中
    return register_filesystem(&fs);
}

// root file system位於VFS樹的最頂層，應該在rootfs上掛載tmpfs, 直接call setup_mount來掛載
int tmpfs_setup_mount(struct filesystem *fs, struct mount *_mount){
    // 設置掛載點的file system
    _mount->fs = fs;
    // 創建root vnode（目錄node），將其設置為掛載點的root
    _mount->root = tmpfs_create_vnode(0, dir_t);
    return 0;
}

// 每個mounted file system都有自己的root vnode, 在mount setup期間建立root vnode
// 每個file system的vnode的internel representation可能不同，使用vnode.internal來指向它
struct vnode* tmpfs_create_vnode(struct mount* _mount, enum fsnode_type type){
    // 分配並init一個vnode結構
    struct vnode *v = kmalloc(sizeof(struct vnode));
    
    // 設置其file operations和vnode operations
    v->f_ops = &tmpfs_file_operations;
    v->v_ops = &tmpfs_vnode_operations;
    v->mount = 0;
    
    // 創建並init一個tmpfs_inode structure, 並return vnode。
    struct tmpfs_inode* inode = kmalloc(sizeof(struct tmpfs_inode));
    memset(inode, 0, sizeof(struct tmpfs_inode));
    inode->type = type;
    inode->data = kmalloc(0x1000);
    v->internal = inode;
    return v;
}

// file operations
// 給定file handle，VFS呼叫對應的write方法從f_pos開始寫入文件，
// 寫入後更新f_pos和size, return讀取的size或error code(如果是特殊文件，則不return)
int tmpfs_write(struct file *file, const void *buf, size_t len)
{
    struct tmpfs_inode *inode = file->vnode->internal;
    // 從file的f_pos位置開始write len bytes data
    // write from f_pos
    memcpy(inode->data + file->f_pos, buf, len);
    
    // update f_pos and size
    // 更新f_pos和file size
    file->f_pos += len;
    if(inode->datasize < file->f_pos) inode->datasize = file->f_pos;
    return len;
}

// 給定file handle，VFS呼叫對應的read方法從f_pos開始讀取文件，
// 讀取後更新f_pos(如果是特殊文件則不update)
// f_pos 不應超過file size
// 一旦read file到達EOF則停止, 並return讀取的size或error code
int tmpfs_read(struct file *file, void *buf, size_t len)
{
    struct tmpfs_inode *inode = file->vnode->internal;
    // if buffer overflow, shrink the request read length    
    // 從file的f_pos位置開始read len bytes data
    // read from f_pos
    if(len+file->f_pos > inode->datasize){
        // 更新f_pos和file size
        len = inode->datasize - file->f_pos;
        memcpy(buf, inode->data + file->f_pos, len);
        file->f_pos += inode->datasize - file->f_pos;
        return len;
    }
    else
    {
        memcpy(buf, inode->data + file->f_pos, len);
        file->f_pos += len;
        return len;
    }
    return -1;
}

// open vnode，並為file建立file handle
int tmpfs_open(struct vnode *file_node, struct file **target){
    (*target)->vnode = file_node;
    (*target)->f_ops = file_node->f_ops;
    (*target)->f_pos = 0;
    return 0;
}

// close and release file handle
int tmpfs_close(struct file *file)
{
    kfree(file);
    return 0;
}

long tmpfs_lseek64(struct file *file, long offset, int whence)
{
    if(whence == SEEK_SET)
    {
        file->f_pos = offset;
        return file->f_pos;
    }
    return -1;
}

// file system迭代directory entry並比較component名稱以尋找target file
// 然後，如果找到file，它會將file的 vnode傳遞給 VFS
// vnode operations
int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct tmpfs_inode *dir_inode = dir_node->internal;
    int child_idx = 0;
    // BFS search tree
    // file system迭代directory entry
    for (; child_idx <= MAX_DIR_ENTRY; child_idx++)
    {
        struct vnode *vnode = dir_inode->entry[child_idx];
        if(!vnode) break;
        struct tmpfs_inode *inode = vnode->internal;
        // 比較component名稱以尋找target file
        if (strcmp(component_name, inode->name) == 0)
        {
            *target = vnode;
            return 0;
        }
    }
    return -1;
}

// 在底層file system上建立regular file，如果file exit則fail, 然後將檔案的 vnode傳至VFS
// 在目錄節點中創建一個新的regular file，並return vnode
// file ops
int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct tmpfs_inode *inode = dir_node->internal;
    if(inode->type!=dir_t)
    {
        uart_sendline("tmpfs create not dir_t\r\n");
        return -1;
    }

    int child_idx = 0;
    for (; child_idx <= MAX_DIR_ENTRY; child_idx++)
    {
        if (!inode->entry[child_idx]) break;
        struct tmpfs_inode *child_inode = inode->entry[child_idx]->internal;
        if (strcmp(child_inode->name,component_name)==0)
        {
            uart_sendline("tmpfs create file exists\r\n");
            return -1;
        }
    }

    if (child_idx > MAX_DIR_ENTRY)
    {
        uart_sendline("DIR ENTRY FULL\r\n");
        return -1;
    }

    if (strlen(component_name) > MAX_FILE_NAME)
    {
        uart_sendline("FILE NAME TOO LONG\r\n");
        return -1;
    }

    struct vnode *_vnode = tmpfs_create_vnode(0, file_t);
    inode->entry[child_idx] = _vnode;

    struct tmpfs_inode *newinode = _vnode->internal;
    strcpy(newinode->name, component_name);

    *target = _vnode;
    return 0;
}
// ---------------------------Basic 1


// dir ops
// 在 tmpfs file system中創建一個目錄
// 檢查並創建新目錄，將其添加到當前目錄的entry中
int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name){
    // dir_node : current目錄的 vnode
    // target : 指向新創建目錄vnode的ptr
    // component_name : 新目錄name
    
    // get current目錄的 inode
    struct tmpfs_inode *inode = dir_node->internal;
    
    // 檢查current node是否為目錄
    if (inode->type != dir_t)
    {
        uart_sendline("tmpfs mkdir not dir_t\r\n");
        return -1;
    }

    int child_idx = 0;
    // 尋找可用的目錄entry index
    for (; child_idx <= MAX_DIR_ENTRY; child_idx++)
    {
        if (!inode->entry[child_idx])
        {
            break;
        }
    }

    // 檢查目錄entry是否full
    if(child_idx > MAX_DIR_ENTRY)
    {
        uart_sendline("DIR ENTRY FULL\r\n");
        return -1;
    }
    
    // 檢查file name是否過長
    if (strlen(component_name) > MAX_FILE_NAME)
    {
        uart_sendline("FILE NAME TOO LONG\r\n");
        return -1;
    }
    
    // 創建新的vnode
    struct vnode* _vnode = tmpfs_create_vnode(0, dir_t);
    inode->entry[child_idx] = _vnode;

    // 設置新目錄的name
    struct tmpfs_inode *newinode = _vnode->internal;
    strcpy(newinode->name, component_name);
    
    // 更新 target ptr
    *target = _vnode;
    return 0;
}

long tmpfs_getsize(struct vnode* vd)
{
    struct tmpfs_inode *inode = vd->internal;
    return inode->datasize;
}
