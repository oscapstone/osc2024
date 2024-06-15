#include "vfs.h"
#include "tmpfs.h"
#include "mini_uart.h"
#include <stddef.h>

#define MAX_FS_NUM 100
#define MAX_PATH_SIZE 255

filesystem* fs_list[MAX_FS_NUM];
struct mount* rootfs;

void filesystem_init() {
	for (int i = 0; i < MAX_FS_NUM; i ++) {
		fs_list[i] = NULL;
	}
	fs_list[0] = my_malloc(sizeof(filesystem));
	fs_list[0] -> name = "tmpfs";
	fs_list[0] -> setup_mount = tmpfs_mount;


	rootfs = my_malloc(sizeof(mount));
	fs_list[0] -> setup_mount(fs_list[0], rootfs);
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
	if (same(pathname, "/")) {
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
            vnode_name[idx] = pathname[i];
            idx++;
        }
    }

    vnode_name[idx] = '\0';
    if(cur_node -> v_ops -> lookup(cur_node, &cur_node, vnode_name) != 0) { //not found
        uart_printf ("File not found\n\r");
        return -1; //get next vnode
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

    tmpfs_node* inode = temp -> internal;
    inode -> type = 2;
    temp -> mount = my_malloc(sizeof(mount));
    
	return fs -> setup_mount(fs, temp -> mount);
}

int vfs_mkdir(const char* pathname) {
	int idx = -1;
	for (int i = 0; i < strlen(pathname); i ++) {
		if (pathname[i] == '/') {
			idx = i;
		}
	}
	if (idx == -1) {
		uart_printf ("vfs_mkdir find string without /\r\n");
		return -1;
	}
	vnode* temp;
	if (vfs_lookup(pathname, &temp) != 0) {
		uart_printf ("mkdir vnode doesn't exist\r\n");
		return -1;
	}
	return temp -> v_ops -> mkdir(temp, &temp, pathname + idx + 1);
}

int vfs_open(const char* pathname, int flags, struct file** target) {
    vnode *temp;
    if(vfs_lookup(pathname, &temp) != 0) {
        if(!(flags & O_CREAT)){
            uart_printf ("Not exist and no OCREATE\r\n");
            return -1;
        }
        
		char path_dir[MAX_PATH_SIZE];
        strcpy(pathname, path_dir, strlen(pathname));
        int idx = -1;
		for(int i = 0; i < strlen(pathname); i++){
            if(pathname[i] == '/') {
                idx = i;
			}
        }
		if (idx == -1) {
			uart_printf ("vfs_open find string without /\r\n");
			return -1;
		}
        path_dir[idx] = '\0';
		// to seperate file to create and directory
        if (vfs_lookup(path_dir, &temp) != 0) {
            uart_printf ("directory not found\r\n");
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
