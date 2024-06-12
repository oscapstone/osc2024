#include "vfs.h"
#include "uart.h"
#include "utils.h"
#include "shell.h"
#include "tmpfs.h"
#include "initramfs.h"
#include "dev_uart.h"
#include "sd_driver.h"

struct mount* rootfs;
struct filesystem fs_register[MAX_FS];
struct directory directories[MAX_DIR];
struct vnode * current_dir;

int register_filesystem(struct filesystem *fs){
    for(int i=0; i< MAX_FS; i++){
        if(fs_register[i].name == 0){
            fs_register[i].name = fs->name;
            fs_register[i].setup_mount = fs->setup_mount;
            return i;
        }
    }
    uart_puts("Filesystem Full!\n\r");
    return -1;
}

int vfs_open(const char* pathname, int flags, struct file** target) { //target: filehandle
    // 1. Lookup pathname
    struct vnode *temp;
    if(vfs_lookup(pathname, &temp) != 0){ // file not found
        if(!(flags & O_CREAT)){//1: O_CREATE
            uart_puts("ERROR OPENING FILE\n\r");
            return -1;
        }
        // 3. Create a new file if O_CREAT is specified in flags and vnode not found
        char path_dir[MAX_PATH];
        strcpy(pathname, path_dir);
        int idx;
        //set the last / into 0 to get next node name for lookup
        for(int i=0; i<strlen(pathname);i++){
            if(pathname[i] == '/')
                idx = i;
        }
        //get the name of folder and lookup for node
        path_dir[idx] = 0;
        if (vfs_lookup(path_dir,&temp)!=0){
            uart_puts("ERROR DIRECTORY NOTFOUND\n\r");
            return -1;
        }

        //set temp to created vnode
        temp -> v_ops -> create(temp, &temp, pathname + idx + 1);//dir, target, filename
    }
    
    *target = allocate_page(4096);
    temp -> f_ops -> open(temp, target);
    (*target) -> flags = flags;
    return 0;
}


int vfs_close(struct file *file) {
    // 1. release the file handle
    // 2. Return error code if fails
    return file->f_ops->close(file);
}

int vfs_write(struct file* file, const void* buf, size_t len) {
    // 1. write len byte from buf to the opened file.
    // 2. return written size or error code if an error occurs.
    return file->f_ops->write(file,buf,len);
}

int vfs_read(struct file* file, void* buf, size_t len) {
    // 1. read min(len, readable size) byte to buf from the opened file.
    // 2. block if nothing to read for FIFO type
    // 2. return read size or error code if an error occurs.
    return file->f_ops->read(file, buf, len);
}

int vfs_mkdir(const char* pathname){
    struct vnode * temp;
    char path_dir[MAX_PATH];
    strcpy(pathname, path_dir);
    int idx;
    for(int i=0; i<strlen(pathname);i++){
        if(pathname[i] == '/')
            idx = i;
    }
    //get the name of folder and lookup for node
    path_dir[idx] = 0;
    if (vfs_lookup(path_dir,&temp)!=0){
        uart_puts("ERROR DIRECTORY NOTFOUND\n\r");
        return -1;
    }

    //set temp to created vnode
    return temp -> v_ops -> mkdir(temp, &temp, pathname + idx + 1);//dir, target, filename
}

int vfs_mount(const char* target, const char* filesystem){
    //mount filesystem on target's vnode -> mount
    struct vnode * temp;
    struct filesystem *fs = 0;
    for(int i=0; i<MAX_FS;i++){
        if(fs_register[i].name == 0)
            break;
        if(strcmp(filesystem, fs_register[i].name) == 0){
            fs = &fs_register[i];
            break;
        }
    }
    if(!fs){
        uart_puts("No such filesystem!\n\r");
        return -1;
    }
    
    if(vfs_lookup(target, &temp) != 0){
        uart_puts("No such directory!\n\r");
        return -1;
    }
    
    struct tmpfs_node * inode = temp -> internal;
    inode -> type = 2;
    temp -> mount = allocate_page(4096);
    
    // uart_puts("Successfully mounted fs ");
    // uart_puts(filesystem);
    // newline();
    return fs -> setup_mount(fs, temp -> mount);
}

int vfs_lookup(const char* pathname, struct vnode** target){
    //in create in open will set / to 0
    if(pathname[0] == 0 || strcmp(pathname, "/") == 0){
        *target = rootfs -> root;
        return 0; 
    }
    int idx = 0;
    char vnode_name[256];
    struct vnode * cur_node = rootfs -> root;
    for(int i=1;i<strlen(pathname);i++){
        if(pathname[i] == '/'){
            //find name in current level
            vnode_name[idx] = '\0';

            if(cur_node -> v_ops -> lookup(cur_node, &cur_node, vnode_name)!=0){
                uart_puts("Parent lookup failed\n\r");
                return -1; //get next vnode
            }
                
            
            while(cur_node -> mount){// check if the vnode is in different file system
                cur_node = cur_node -> mount -> root;
            }
            idx = 0;
        }
        else{
            vnode_name[idx] = pathname[i];
            idx++;
        }
    }

    vnode_name[idx] = 0;
    if(cur_node -> v_ops -> lookup(cur_node, &cur_node, vnode_name)!=0){ //not found
        //uart_puts("File not found\n\r");
        return -1; //get next vnode
    }
        
    
    while(cur_node -> mount){
        cur_node = cur_node -> mount -> root;
    }
    
    *target = cur_node;
    
    return 0;
}

void init_rootfs(){
    // cannot use vfs_mount because starting point is vfs_root, and it has nothing yet
    // will mount at rootfs -> root -> mount -> root instead of rootfs -> root
    // reg_tmpfs();
    // vfs_mount("/", "tmpfs");
    // mount tmpfs on rootfs
    int idx = reg_tmpfs();
    fs_register[idx].setup_mount(&fs_register[idx], rootfs);

    sd_init();
    reg_fat32();
    vfs_mkdir("/boot");
    vfs_mount("/boot", "fat32");
    struct file * temp;
    //vfs_lookup("/boot/KERNEL8.IMG", &temp);
    char buf[100];
    memset(buf, 100);
    vfs_open("/boot/FAT_R.TXT",0, &temp);
    vfs_read(temp, buf, 100);
    uart_puts(buf);
    
    reg_initramfs();
    vfs_mkdir("/initramfs");
    vfs_mount("/initramfs", "initramfs");

    reg_uart_dev();
    vfs_mkdir("/dev");
    vfs_mkdir("/dev/uart");
    vfs_mount("/dev/uart", "uart");

    current_dir = rootfs -> root; //no_use
}


// just for fun, not good
void pwd(){ 
    uart_puts((char *) (current_dir -> internal));
    newline();
}

int vfs_ls(char * dir_name){
    struct vnode * temp;
    if (vfs_lookup(dir_name,&temp)!=0){
        uart_puts("ERROR DIRECTORY NOTFOUND\n\r");
        return -1;
    }
    struct tmpfs_node * internal = temp -> internal;
    uart_puts("Name\t\tType\n\r");
    for(int i=0; i < MAX_ENTRY; i++){
        if(internal -> entry[i] == 0)
            break;
        struct tmpfs_node * inode = internal -> entry[i] -> internal;
        uart_puts(inode -> name);
        uart_puts("\t\t");
        if(inode -> type == 3){
            uart_puts("file");
        }
        else if(inode -> type == 2){
            uart_puts("mount");
        }
        else if (inode -> type == 1){
            uart_puts("folder");
        }
        newline();
    }
    return 0;
}

#include "thread.h"
extern thread * get_current();
void shell_with_dir(){
    //will not show full path name, need to try other way, but current_dir can keep for ls
    //can still use char[MAX_PATH], also good for cd implementation
    uart_send('\r');
    uart_puts(get_current() -> work_dir);
    uart_puts(" # ");
}

void cd(char * relative){
    const char * ndir = path_convert(relative, get_current() -> work_dir);
    if(ndir[1] == '/') 
        strcpy(ndir+1, get_current()->work_dir);
    else
        strcpy(ndir, get_current()->work_dir);
}

const char * path_convert(char * relative, char * work_dir){
    //case absolute
    if(relative[0] == '/'){
        return relative;
    }
    
    char temp[256];
    const char * ret = malloc(255);
    //case current folder
    if(relative[0] == '.' && relative[1] == '/'){
        //copy workdir
        for(int i=0;i<strlen(work_dir);i++){
            temp[i] = work_dir[i];
        }
        temp[strlen(work_dir)] = 0;
        int base = strlen(temp);
        for(int i = 1; i< strlen(relative);i++){
            temp[base + i - 1] = relative[i];
        }
        temp[base + strlen(relative) - 1] = 0;
        for(int i = 0; i< strlen(temp) ;i++){
            if(temp[i] == '\n')
                temp[i] = 0;
        }
        if(temp[0] == '/' && temp[1] == '/'){
            strcpy(temp + 1, ret);
            return ret;
        }
        strcpy(temp, ret);
        return ret;
    }

    //case last folder
    if(relative[0] == '.' && relative[1] == '.'){
        int idx = -1;
        for(int i = 0;i<strlen(work_dir); i++){
            temp[i] = work_dir[i];
            if(work_dir[i]=='/'){
                idx = i;
            }
        }
        temp[idx] = 0;
        int base = strlen(temp);

        for(int i=2;i<strlen(relative); i++){
            temp[base - 2 + i] = relative[i];
        }
        temp[base - 2 + strlen(relative)] = 0;
        strcpy(temp, ret);
        return ret;
    }

    for(int i=0;i<strlen(work_dir);i++){
        temp[i] = work_dir[i];
    }
    temp[strlen(work_dir)] = '/';
    
    for(int i = 0; i<strlen(relative);i++){
        temp[strlen(work_dir)+1+i] = relative[i];
    }
    strcpy(temp, ret);
    return ret;
}