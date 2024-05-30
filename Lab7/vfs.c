#include "vfs.h"
#include "uart.h"

char current_dir[MAX_PATH];
struct mount* rootfs;
struct filesystem fs_register[MAX_FS];
struct directory directories[MAX_DIR];

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

// int register_filesystem(struct filesystem* fs) {
//   // register the file system to the kernel.
//   // you can also initialize memory pool of the file system here.
// }

// int vfs_open(const char* pathname, int flags, struct file** target) {
//   // 1. Lookup pathname
//   // 2. Create a new file handle for this vnode if found.
//   // 3. Create a new file if O_CREAT is specified in flags and vnode not found
//   // lookup error code shows if file exist or not or other error occurs
//   // 4. Return error code if fails
// }

// int vfs_close(struct file* file) {
//   // 1. release the file handle
//   // 2. Return error code if fails
// }

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
// int vfs_lookup(const char* pathname, struct vnode** target);

void pwd(){ 
    uart_puts(current_dir);
    newline();
}

void shell_with_dir(){
    uart_send('\r');
    uart_puts(current_dir);
    uart_puts(" # ");
}