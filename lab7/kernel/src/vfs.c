#include "vfs.h"
#include "tmpfs.h"
#include "memory.h"
#include "string.h"
#include "uart1.h"
#include "initramfs.h"
#include "dev_uart.h"
#include "dev_framebuffer.h"

struct mount *rootfs;
struct filesystem reg_fs[MAX_FS_REG];
struct file_operations reg_dev[MAX_DEV_REG];


// 將檔案系統register到kernel
// 可以在這裡initial file system的memory pool
int register_filesystem(struct filesystem *fs)
{
    for (int i = 0; i < MAX_FS_REG;i++)
    {
        if(!reg_fs[i].name)
        {
            reg_fs[i].name = fs->name;
            reg_fs[i].setup_mount = fs->setup_mount;
            return i;
        }
    }
    return -1;
}

int register_dev(struct file_operations *fo)
{
    for (int i = 0; i < MAX_FS_REG; i++)
    {
        if (!reg_dev[i].open)
        {
            // return unique id for the assigned device
            reg_dev[i] = *fo;
            return i;
        }
    }
    return -1;
}

struct filesystem* find_filesystem(const char* fs_name)
{
    for (int i = 0; i < MAX_FS_REG; i++)
    {
        if (strcmp(reg_fs[i].name,fs_name)==0)
        {
            return &reg_fs[i];
        }
    }
    return 0;
}


// file ops
int vfs_open(const char *pathname, int flags, struct file **target)
{
    // 1. 查詢 pathname
    // 3. 如果在 flags 中指定了 O_CREAT 且未找到 vnode，則建立新file
    struct vnode *node;
    if (vfs_lookup(pathname, &node) != 0 && (flags & O_CREAT))
    {
        // grep all of the directory path
        int last_slash_idx = 0;
        for (int i = 0; i < strlen(pathname); i++)
        {
            if(pathname[i]=='/')
            {
                last_slash_idx = i;
            }
        }

        char dirname[MAX_PATH_NAME+1];
        strcpy(dirname, pathname);
        dirname[last_slash_idx] = 0;
        // update dirname to node
        if (vfs_lookup(dirname,&node)!=0)
        {
            uart_sendline("cannot ocreate no dir name\r\n");
            return -1;
        }
        // create a new file node on node, &node is new file, 3rd arg is filename
        node->v_ops->create(node, &node, pathname+last_slash_idx+1);
        *target = kmalloc(sizeof(struct file));
        // attach opened file on the new node
        node->f_ops->open(node, target);
        (*target)->flags = flags;
        return 0;
    }
    else // 2. 如果找到，則為此 vnode 建立一個新file handle
    {
        // attach opened file on the node
        *target = kmalloc(sizeof(struct file));
        node->f_ops->open(node, target);
        (*target)->flags = flags;
        return 0;
    }

    // 尋找錯誤代碼顯示file是否存在或發生其他錯誤
    // 4. 如果fail則return error code
    return -1;
}

// file ops
int vfs_close(struct file *file)
{
    // 1. release the file handle
    // 2. 如果fail則return error code
    file->f_ops->close(file);
    return 0;
}

// file ops
int vfs_write(struct file *file, const void *buf, size_t len)
{
    // 1. 將 buf 中的 len 位元組寫入開啟的檔案
    // 2. 如果發生error，則return寫入的size或error code
    return file->f_ops->write(file,buf,len);
}

// file ops
int vfs_read(struct file *file, void *buf, size_t len)
{
    // 1. 從opened file read min(len, readable size) bytes到 buf
    // 2. 如果沒有 FIFO 類型可讀取的內容，則block
    // 3. 如果發生 error, return read size or error code
    return file->f_ops->read(file, buf, len);
}


// Basic 2
// 在底層file system上建立一個directory，與建立regular file相同
// file ops
int vfs_mkdir(const char *pathname)
{
    char dirname[MAX_PATH_NAME] = {};    // before add folder
    char newdirname[MAX_PATH_NAME] = {}; // after  add folder

    // search for last directory
    // 找到最後一個 "/" 前的dirname和 "/" 後的new dirname
    int last_slash_idx = 0;
    for (int i = 0; i < strlen(pathname); i++)
    {
        if (pathname[i] == '/')
        {
            last_slash_idx = i;
        }
    }

    memcpy(dirname, pathname, last_slash_idx);
    strcpy(newdirname, pathname + last_slash_idx + 1);


    // create new directory if upper directory is found
    struct vnode *node;
    // 用vfs_lookup找dirname所在的目錄node
    if(vfs_lookup(dirname,&node)==0){
        // 找到則用該node的mkdir方法來創建新dir
        // node is the old dir, &node is new dir
        node->v_ops->mkdir(node, &node, newdirname);
        return 0;
    }

    uart_sendline("vfs_mkdir cannot find pathname");
    return -1;
}

// 與mount root file system相同，能夠mount在任何vnode上
int vfs_mount(const char *target, const char *filesystem)
{
    struct vnode *dirnode;
    // search for the target filesystem
    // 用find_filesystem找指定名稱的file system
    struct filesystem *fs = find_filesystem(filesystem);
    
    // 找不到
    if(!fs){
        uart_sendline("vfs_mount cannot find filesystem\r\n");
        return -1;
    }
    
    // 用vfs_lookup找target目錄node(target)
    if(vfs_lookup(target, &dirnode)==-1){
        uart_sendline("vfs_mount cannot find dir\r\n");
        return -1;
    }else{
        // 找到則為target目錄node分配一個 mount structure
        // mount fs on dirnode
        dirnode->mount = kmalloc(sizeof(struct mount));
        // 用fs的 setup_mount方法來set mount
        fs->setup_mount(fs, dirnode->mount);
    }
    return 0;
}

// VFS api以pathname作為參數(目前是絕對路徑"/")，透過traverse vnode來找出pathname
// 從root file system的root vnode開始
// lookup必須cross mounting point, 
// 對於已mount的 vnode，VFS應轉到已mount file system的root vnode
// 根據pathname find對應的vnode
int vfs_lookup(const char *pathname, struct vnode **target)
{
    // if no path input, return root
    // 如果pathname為空，return root file system的root node
    if(strlen(pathname)==0)
    {
        *target = rootfs->root;
        return 0;
    }

    struct vnode *dirnode = rootfs->root;
    char component_name[MAX_FILE_NAME+1] = {};
    int c_idx = 0;
    
    // deal with directory
    // 從root node，traverse path的每一部分
    for (int i = 1; i < strlen(pathname); i++){
        if (pathname[i] == '/'){
            component_name[c_idx++] = 0;
            // if fs's v_ops error, return -1
            // 用lookup找對應的child node
            if (dirnode->v_ops->lookup(dirnode, &dirnode, component_name) != 0) return -1;
            // redirect to mounted filesystem
            // 如果node mount了另一個file system，redirect到mount的root node
            while (dirnode->mount){
                dirnode = dirnode->mount->root;
            }
            c_idx = 0;
        }else{
            component_name[c_idx++] = pathname[i];
        }
    }

    // deal with file
    component_name[c_idx++] = 0;
    // if fs's v_ops error, return -1
    // 用lookup找對應的child node
    if (dirnode->v_ops->lookup(dirnode, &dirnode, component_name) != 0) return -1;
    // redirect to mounted filesystem
    // 如果node mount了另一個file system，redirect到mount的root node
    while (dirnode->mount){
        dirnode = dirnode->mount->root;
    }
    // return file's vnode
    // return找到的target node
    *target = dirnode;

    return 0;
}
// ---------------------------Basic 2


// Advanced
// for device operations only
// 在 VFS中創建一個device node，並將其file operation設置為對應的operation
int vfs_mknod(char* pathname, int id)
{
    struct file* f = kmalloc(sizeof(struct file));
    // create leaf and its file operations
    vfs_open(pathname, O_CREAT, &f);
    f->vnode->f_ops = &reg_dev[id];
    vfs_close(f);
    return 0;
}

// 初始化root file system並mount不同類型的file system
void init_rootfs()
{
    // tmpfs
    // 註冊 tmpfs並設置為root file system
    int idx = register_tmpfs();
    rootfs = kmalloc(sizeof(struct mount));
    reg_fs[idx].setup_mount(&reg_fs[idx], rootfs);

    // initramfs
    // 創建並mount initramfs
    vfs_mkdir("/initramfs");
    register_initramfs();
    vfs_mount("/initramfs","initramfs");

    // 創建/dev directory，並在/dev創建 UART和framebuffer device node
    // dev_fs
    vfs_mkdir("/dev");
    
    // Advanced 1
    int uart_id = init_dev_uart();
    vfs_mknod("/dev/uart", uart_id);
    
    // Advanced 2
    int framebuffer_id = init_dev_framebuffer();
    vfs_mknod("/dev/framebuffer", framebuffer_id);
}

void vfs_test()
{
    // test read/write
    vfs_mkdir("/lll");
    vfs_mkdir("/lll/ddd");
    // test mount
    vfs_mount("/lll/ddd", "tmpfs");

    struct file* testfilew;
    struct file *testfiler;
    char testbufw[0x30] = "ABCDEABBBBBBDDDDDDDDDDD";
    char testbufr[0x30] = {};
    vfs_open("/lll/ddd/ggg", O_CREAT, &testfilew);
    vfs_open("/lll/ddd/ggg", O_CREAT, &testfiler);
    vfs_write(testfilew, testbufw, 10);
    vfs_read(testfiler, testbufr, 10);
    uart_sendline("%s",testbufr);

    struct file *testfile_initramfs;
    vfs_open("/initramfs/get_simpleexec.sh", O_CREAT, &testfile_initramfs);
    vfs_read(testfile_initramfs, testbufr, 30);
    uart_sendline("%s", testbufr);
}

char *get_absolute_path(char *path, char *curr_working_dir)
{
    // if relative path -> add root path
    if(path[0] != '/')
    {
        char tmp[MAX_PATH_NAME];
        strcpy(tmp, curr_working_dir);
        if(strcmp(curr_working_dir,"/")!=0)strcat(tmp, "/");
        strcat(tmp, path);
        strcpy(path, tmp);
    }

    char absolute_path[MAX_PATH_NAME+1] = {};
    int idx = 0;
    for (int i = 0; i < strlen(path); i++)
    {
        // meet /..
        if (path[i] == '/' && path[i+1] == '.' && path[i+2] == '.')
        {
            for (int j = idx; j >= 0;j--)
            {
                if(absolute_path[j] == '/')
                {
                    absolute_path[j] = 0;
                    idx = j;
                }
            }
            i += 2;
            continue;
        }

        // ignore /.
        if (path[i] == '/' && path[i+1] == '.')
        {
            i++;
            continue;
        }

        absolute_path[idx++] = path[i];
    }
    absolute_path[idx] = 0;

    return strcpy(path, absolute_path);
}

