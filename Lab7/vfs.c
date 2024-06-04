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
// int vfs_mount(const char* target, const char* filesystem);

int vfs_lookup(const char* pathname, struct vnode** target){
    if(pathname[0] == 0 || strcmp(pathname, "/") == 0){
        *target = rootfs -> root;
        return 0; 
    }
    int idx = 0;
    char vnode_name[256];
    struct vnode * cur_node = rootfs -> root;
    for(int i=1;i<strlen(pathname);i++){
        if(pathname[i] == '/'){
            //not sure use full path or only name
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
    int idx = reg_tmpfs();
    fs_register[idx].setup_mount(&fs_register[idx], rootfs);
    current_dir = rootfs -> root;
    please_nobug();
}

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


void please_nobug(){
    struct file* test; //file handle
    struct file* test2;
    // vfs_open("/file1", 1, &test);

    split_line();
    vfs_open("/jerry", 1, &test);
    struct file* test3;
    vfs_open("/file3", 1, &test);
    vfs_open("/file3", 0, &test2);
    struct vnode * node;

    split_line();
    int ret = vfs_lookup("/jerry", &node);
    uart_int(ret);
    newline();
    split_line();


    vfs_mkdir("/folder1");
    split_line();

    ret = vfs_lookup("/jerry", &node);
    uart_int(ret);
    newline();
    split_line();


    vfs_open("/folder1/file1", 1, &test3);
    split_line();
    
    const char * fn = "/jerry";
    ret = vfs_lookup(fn, &node);
    uart_int(ret);
    newline();
    split_line();
    char * buf = "hello world!\n";
    vfs_write(test, buf, 7);
    char buf2[10];
    vfs_read(test2, buf2, 5);
    uart_puts(buf2);
    newline();

    
    // struct tmpfs_node * nd = test -> vnode -> internal;
    // uart_puts(nd -> data);
    // newline();

    struct tmpfs_node * inode = node -> internal;
    uart_puts(inode -> name);
    newline();
    vfs_ls("/");

}

void shell_with_dir(){
    //will not show full path name, need to try other way, but current_dir can keep for ls
    //can still use char[MAX_PATH], also good for cd implementation
    uart_send('\r');
    uart_puts((char *) (current_dir -> internal));
    uart_puts(" # ");
}