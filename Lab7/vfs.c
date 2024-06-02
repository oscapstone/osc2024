#include "vfs.h"
#include "uart.h"
#include "utils.h"
#include "shell.h"
#include "tmpfs.h"

//need ** so that can return handle and status

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
        if(flags != 1){//1: O_CREATE
            uart_puts("ERROR OPENING FILE\n\r");
            return -1;
        }
        // 3. Create a new file if O_CREAT is specified in flags and vnode not found
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
    file->f_ops->close(file);
    return 0;
}


// int vfs_write(struct file* file, const void* buf, size_t len) {
//   // 1. write len byte from buf to the opened file.
//   // 2. return written size or error code if an error occurs.
// }

// int vfs_read(struct file* file, void* buf, size_t len) {
//   // 1. read min(len, readable size) byte to buf from the opened file.
//   // 2. block if nothing to read for FIFO type
//   // 2. return read size or error code if an error occurs.
// }

// int vfs_mkdir(const char* pathname);
// int vfs_mount(const char* target, const char* filesystem);

int vfs_lookup(const char* pathname, struct vnode** target){
    int idx = 0;
    char vnode_name[256];
    struct vnode * cur_node = rootfs -> root;
    for(int i=1;i<strlen(pathname);i++){
        if(pathname[i] == '/'){
            //not sure use full path or only name
            vnode_name[idx] = '\0';

            if(cur_node -> v_ops -> lookup(cur_node, &cur_node, vnode_name)!=0)
                return -1; //get next vnode
            
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

    vnode_name[idx+1] = 0;
    if(cur_node -> v_ops -> lookup(cur_node, &cur_node, vnode_name)!=0) //not found
        return -1; //get next vnode
    
    while(cur_node -> mount){
        cur_node = cur_node -> mount -> root;
    }
    *target = cur_node;
    
    return 0;
}

void init_rootfs(){
    int idx = reg_tmpfs();
    fs_register[idx].setup_mount(&fs_register[idx], rootfs);
    current_dir = rootfs -> root;
}

void pwd(){ 
    uart_puts((char *) (current_dir -> internal));
    newline();
}

void shell_with_dir(){
    //will not show full path name, need to try other way, but current_dir can keep for ls
    //can still use char[MAX_PATH], also good for cd implementation
    uart_send('\r');
    uart_puts((char *) (current_dir -> internal));
    uart_puts(" # ");
}