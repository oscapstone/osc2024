# OSC2024 Lab7
## VFS Introduction
An abstract layer which provide a uniform file interface for different filesystems. With the API provided by VFS, application can utilize different filesystem easily using open, read, write and so on. Also, VFS allows user to mount filesystem in any point.

## Root File System
The whole tree structure is built with vnodes, every file/folder has a represented vnode, the specific metadata can be found in vnode -> internal (which might differ from filesystems). Also, when a file system is mounted on a vnode, the future lookup of the mounted vnode will become vnode -> mount -> root (new vnode)
### Initialize
Steps to mount tmpfs on root (root is a mount: rootfs -> root):
1. register filesystem tmpfs to file_systems.
2. use fs->mount to mount on rootfs (set mount -> root to a newly created node)

### Data Structure
#### vnode
```
struct vnode {
  struct mount* mount;
  struct vnode_operations* v_ops;
  struct file_operations* f_ops;
  void* internal; //structure of node should start with name for printing current dir
};
```
#### file
containt the vnode of the file and the position, operations
```
struct file {
  struct vnode* vnode;
  size_t f_pos;  // RW position of this file handle
  struct file_operations* f_ops;
  int flags;
};
```
#### tmpfs (internal node)
```
struct tmpfs_node{
    char name[MAX_PATH];
    int type; // directory, mount, file
    struct vnode * entry[MAX_ENTRY]; //next level vnode
    char * data; // data
    int size;
};
```

## VFS Methods
### File Operations
#### vfs_open (file as target)
Look up the vnode of the path and put the target file into file handle (using f_ops open). If the file is not exist and O_CREATE is set, find the vnode of the folder and create the file using v_ops in the folder, then do open.
#### vfs_close/write/read
Simply call f_ops of the handle.
### vnode Operations
#### vfs_mkdir
Similar as create of vfs_open, just use v_ops mkdir instead of v_ops create
#### vfs_mount
Given target place(path), mount filesystem. Lookup target path's vnode and filesystem, then call filesystem's setup to the node's mount.
#### vfs_lookup (vnode as target)
Start from root, copy name of path until meet next /, use node's lookup to find next node with the name in internal. repeat until find last node, and move to the root of mount if meet another filesystem.
(vnode1 -> vnode2(with mount) -> mount -> root)

### tmpfs Methods
#### tmpfs_write/read
Use file -> vnode -> internal to get the data and write/read data. f_pos is to point to current reading point.
#### tmpfs_close
Simply free the file handle
#### tmpfs_open
set vnode to the handle and initialize f_pos 
#### tmpfs_lookup
lookup the entries' name in the vnode, set to target if found
#### tmpfs_create/mkdir
find an empty entry and create a new vnode(with internal initialized), set name.

## Multitask VFS
### thread initialization
Set working directory to / and file_table with 16 entries. (also open three uart as stdin/stdout/stderror) into table. When open a file, find an empty entry in table, and use int fd to search for the table later.
### relative path
* if start with /, absolute.
* start with ./, concat /.... with work dir (return from index 1 if workdir is root)
* start with ../, make the last / in workdir to 0, then concat from that point with /....
* start with "", add a / and concat (return from index 1 if workdir is root)

## initramfs
Almost the same as tmpfs, but initialize all entry of initramfs during mount, create a vnode for each file inside. (read only) The data in the internal node will become the pointer of the place in initramfs and the size is also parsed by cpio. 
### exec
find the vnode of the file, and copy data with size get from internal node.


## UART device
Create directory and mount uart, only read write open close. Write and read uses uart_send and uart_getc to send and recieve/send from/to buffer.

## NOTE
* Use const char to avoid string disappeared in stack
* Use ** target so that can get both target and return value. 
* VFS 使得應用程式可以在不同的檔案系統上進行操作，而無需關心底層檔案系統的具體實現細節。這種設計有助於操作系統支持多種檔案系統，並且提高了程式的可移植性。