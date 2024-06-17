#include "vfs.h"
#include "tmpfs.h"
#include "initramfs.h"
#include "framebufferfs.h"
#include "uartfs.h"
#include "mini_uart.h"
#include "helper.h"
#include "alloc.h"
#include <stddef.h>

filesystem* fs_list[MAX_FS_NUM];
mount* rootfs;

void filesystem_init() {
	for (int i = 0; i < MAX_FS_NUM; i ++) {
		fs_list[i] = NULL;
	}
	fs_list[0] = my_malloc(sizeof(filesystem));
	fs_list[0] -> name = "tmpfs";
	fs_list[0] -> setup_mount = tmpfs_mount;

	rootfs = my_malloc(sizeof(mount));
	fs_list[0] -> setup_mount(fs_list[0], rootfs);

	fs_list[1] = my_malloc(sizeof(filesystem));
	fs_list[1] -> name = "initramfs";
	fs_list[1] -> setup_mount = initramfs_mount;

	vfs_mkdir("/initramfs");
	vfs_mount("/initramfs", "initramfs");
	
	fs_list[2] = my_malloc(sizeof(filesystem));
	fs_list[2] -> name = "uartfs";
	fs_list[2] -> setup_mount = uart_dev_mount;
	
	vfs_mkdir("/dev");
	vfs_mkdir("/dev/uart");
	vfs_mount("/dev/uart", "uartfs");

	fs_list[3] = my_malloc(sizeof(filesystem));
	fs_list[3] -> name = "framebufferfs";
	fs_list[3] -> setup_mount = framebuffer_mount;
	
	vfs_mkdir("/dev/framebuffer");
	vfs_mount("/dev/framebuffer", "framebufferfs");
}

int register_filesystem(filesystem* fs) {
	for (int i = 0; i < MAX_FS_NUM; i ++) {
		if (fs_list[i] == NULL) {
			fs_list[i] = fs;
			return i;
		}
	}
	uart_printf ("Fail to register fs\r\n");
	return -1;
}

int vfs_lookup(const char* pathname, struct vnode** target) {
	uart_printf ("[VFS]looking up %s\r\n", pathname);
	if (pathname[0] == 0 || same(pathname, "/")) {
		*target = rootfs -> root;
		return 0;
	}
	int idx = 0;
    char vnode_name[MAX_PATH_SIZE];
    vnode* cur_node = rootfs -> root;
    for(int i = 1; i < strlen(pathname); i ++){
        if(pathname[i] == '/'){
            vnode_name[idx] = '\0';
            if(cur_node -> v_ops -> lookup(cur_node, &cur_node, vnode_name) != 0) {
                uart_printf ("Parent lookup failed\n\r");
                return -1;
            }
            while(cur_node -> mount){
                cur_node = cur_node -> mount -> root;
            }
            idx = 0;
        }
        else{
            vnode_name[idx ++] = pathname[i];
        }
    }

    vnode_name[idx] = '\0';
    if(cur_node -> v_ops -> lookup(cur_node, &cur_node, vnode_name) != 0) { //not found
        uart_printf ("File not found\n\r");
        return -1;
    }
    
    while(cur_node -> mount) {
        cur_node = cur_node -> mount -> root;
    }
    
    *target = cur_node;
    return 0;
}

int vfs_mount(const char* target, const char* filesystem){
    //mount filesystem on target's vnode -> mount
    vnode* temp;
    struct filesystem *fs = 0;
    for(int i = 0; i < MAX_FS_NUM; i ++) {
    	if (fs_list[i] == NULL) continue;
		if (same(fs_list[i] -> name, filesystem)) {
			fs = fs_list[i];
		}
	}
    if(!fs){
        uart_printf ("No such filesystem!\n\r");
        return -1;
    }

    if(vfs_lookup(target, &temp) != 0){
        uart_printf ("No such directory!\n\r");
        return -1;
    }

    temp -> mount = my_malloc(sizeof(mount));
    
	return fs -> setup_mount(fs, temp -> mount);
}

int vfs_mkdir(const char* pathname) {
	int idx = -1;
	char path_dir[MAX_PATH_SIZE];
	strcpy(pathname, path_dir, strlen(pathname));
	for (int i = 0; i < strlen(pathname); i ++) {
		if (path_dir[i] == '/') {
			idx = i;
		}
	}
	if (idx == -1) {
		uart_printf ("vfs_mkdir find string without /\r\n");
		return -1;
	}
	path_dir[idx] = '\0';
	vnode* temp;
	if (vfs_lookup(path_dir, &temp) != 0) {
		uart_printf ("mkdir vnode doesn't exist\r\n");
		return -1;
	}
	return temp -> v_ops -> mkdir(temp, &temp, pathname + idx + 1);
}

int vfs_open(const char* pathname, int flags, file** target) {
    vnode *temp;
    if(vfs_lookup(pathname, &temp) != 0) {
        if(!(flags & O_CREAT)){
            uart_printf ("[VFS]Not exist and no OCREATE\r\n");
            return -1;
        }

		uart_printf ("[VFS]Fail to find %s, create one\r\n", pathname);
        
		char path_dir[MAX_PATH_SIZE];
        strcpy(pathname, path_dir, strlen(pathname));
        int idx = -1;
		for(int i = 0; i < strlen(pathname); i++){
            if(pathname[i] == '/') {
                idx = i;
			}
        }
		if (idx == -1) {
			uart_printf ("[VFS]vfs_open find string without /\r\n");
			return -1;
		}
        path_dir[idx] = '\0';
		// to seperate file to create and directory
        if (vfs_lookup(path_dir, &temp) != 0) {
            uart_printf ("[VFS]directory not found\r\n");
            return -1;
        }

        temp -> v_ops -> create(temp, &temp, pathname + idx + 1);
    }
    
    *target = my_malloc(sizeof(file));
    temp -> f_ops -> open(temp, target);
    (*target) -> flags = flags;
    return 0;
}


int vfs_close(struct file *file) {
    // 1. release the file handle
    // 2. Return error code if fails
    return file -> f_ops -> close(file);
}

int vfs_write(struct file* file, const void* buf, size_t len) {
    // 1. write len byte from buf to the opened file.
    // 2. return written size or error code if an error occurs.
    return file -> f_ops -> write(file, buf, len);
}

int vfs_read(struct file* file, void* buf, size_t len) {
    // 1. read min(len, readable size) byte to buf from the opened file.
    // 2. block if nothing to read for FIFO type
    // 2. return read size or error code if an error occurs.
    return file -> f_ops -> read(file, buf, len);
}


const char* to_absolute(char* relative, char* work_dir){
    //case absolute
    if(relative[0] == '/'){
        return relative;
    }

    char temp[256];
    const char * ret = my_malloc(255);
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
            strcpy_to0(temp + 1, ret);
            return ret;
        }
        strcpy_to0(temp, ret);
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
        strcpy_to0(temp, ret);
        return ret;
    }

    for(int i=0;i<strlen(work_dir);i++){
        temp[i] = work_dir[i];
    }
    temp[strlen(work_dir)] = '/';

    for(int i = 0; i<strlen(relative);i++){
        temp[strlen(work_dir)+1+i] = relative[i];
    }
    strcpy_to0(temp, ret);
    return ret;
}
