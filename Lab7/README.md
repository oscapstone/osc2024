# Lab7
[class webpage](https://nycu-caslab.github.io/OSC2024/labs/lab7.html)
---
## Basic Exercises
** The ```pool_alloc``` seemed to have problem in small size memory allocation, need more study. Use 4096 for now. **
** The ```memzero``` has some issues. Use ```mem_set``` first
### Basic Exercise 1 - 
+ background

### Basic Exercise 2 - Multi-level VFS
+ Background
### Basic Exercise 3 - Multitask VFS
### Basic Exercise 4 - /initramfs
+ First register a tmpfs, then make rootfs as tmpfs
    1. ```filesystems[0]``` become tmpfs
    2. call ```tmpfs.setup_mount```, this will create its vnode and corresponding tmpfs inode. Assign rootfs to a tmpfs directory using them
+ ```vfs_mkdir```
    1. Find the directory path(the path before last slash) and file name, 
    2. Then using ```vfs_lookup``` to find the vnode of directory path
    3. call mkdir method of corresponding file system
## Advanced Exercises
### Advanced Exercise 1 - /dev/uart
+ My process PCB will lead to content corruption if the elements inside is placed wrong, need more study
### Advanced Exercise 2 - /dev/framebuffer


