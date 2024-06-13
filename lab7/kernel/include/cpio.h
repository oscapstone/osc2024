#ifndef __CPIO_H
#define __CPIO_H

#define MAX_FILE_SIZE 16

#include "vfs.h"

typedef struct cpio_newc_header
{
    char c_magic[6];
    char c_ino[8];
    char c_mode[8];
    char c_uid[8];
    char c_gid[8];
    char c_nlink[8];
    char c_mtime[8];
    char c_filesize[8];
    char c_devmajor[8];
    char c_devminor[8];
    char c_rdevmajor[8];
    char c_rdevminor[8];
    char c_namesize[8];
    char c_check[8];
}FILE_HEADER;

typedef struct FILE
{
    FILE_HEADER *file_header;
    char *path_name;
    char *file_content;
} FILE;

extern FILE file_arr[MAX_FILE_SIZE];
extern int file_num;
extern unsigned long long cpio_address;

void build_file_arr();

int initramfs_setup_mount(struct filesystem *fs, struct mount *mount);
int initramfs_write(struct file *file, const void *buf, size_t len);
int initramfs_read(struct file *file, void *buf, size_t len);
struct dentry *initramfs_lookup(struct inode *dir, const char *component_name);
int initramfs_create(struct inode *dir, struct dentry *dentry, int mode);
int initramfs_mkdir(struct inode *dir, struct dentry *dentry);

void traverse_file();
void look_file_content(char *pathname);

#endif