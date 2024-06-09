#include "utils.h"
#include "uart.h"
#include "allocator.h"
#include "cpio.h"

FILE file_arr[MAX_FILE_SIZE];
int file_num = 0;
unsigned long long cpio_address;

static struct file_operations initramfs_f_ops = {
    .write = initramfs_write,
    .read = initramfs_read};

static struct inode_operations initramfs_i_ops = {
    .lookup = initramfs_lookup,
    .create = initramfs_create,
    .mkdir = initramfs_mkdir};

void build_file_arr()
{
    int idx = 0;
    unsigned char *address = (unsigned char *)cpio_address;
    // When pathname is "TRAILER!!!". it means it's the end of file.
    while (my_strcmp((char *)(address + sizeof(FILE_HEADER)), "TRAILER!!!") != 0)
    {
        // Pointer to corresponded address.
        file_arr[idx].file_header = (FILE_HEADER *)address;
        unsigned long long header_path_name_size = sizeof(FILE_HEADER) + given_size_hex_atoi(file_arr[idx].file_header->c_namesize, 8);
        unsigned long long file_content_size = given_size_hex_atoi(file_arr[idx].file_header->c_filesize, 8);

        // Align to 4
        header_path_name_size += header_path_name_size % 4 == 0 ? 0 : 4 - header_path_name_size % 4;
        file_content_size += file_content_size % 4 == 0 ? 0 : 4 - file_content_size % 4;

        file_arr[idx].path_name = (char *)(address + sizeof(FILE_HEADER));
        file_arr[idx].file_content = (char *)(address + header_path_name_size);

        address += header_path_name_size + file_content_size;
        idx++;
    }
    file_num = idx;
};

int initramfs_setup_mount(struct filesystem *fs, struct mount *mount)
{
    mount->root = kmalloc(sizeof(struct dentry));
    my_strcpy(mount->root->d_name, "/");
    mount->root->d_parent = NULL;

    mount->root->d_inode = kmalloc(sizeof(struct inode));
    mount->root->d_inode->f_ops = &initramfs_f_ops;
    mount->root->d_inode->i_ops = &initramfs_i_ops;
    mount->root->d_inode->i_dentry = mount->root;
    mount->root->d_inode->internal = NULL;

    for (int i = 0; i < 16; i++)
        mount->root->d_subdirs[i] = NULL;

    for (int i = 0; i < file_num; i++)
    {
        mount->root->d_subdirs[i] = kmalloc(sizeof(struct dentry));
        mount->root->d_subdirs[i]->d_inode = kmalloc(sizeof(struct inode));
        mount->root->d_subdirs[i]->d_inode->i_dentry = mount->root->d_subdirs[i];
        mount->root->d_subdirs[i]->d_inode->f_ops = &initramfs_f_ops;
        mount->root->d_subdirs[i]->d_inode->i_ops = &initramfs_i_ops;
        mount->root->d_subdirs[i]->d_inode->internal = &(file_arr[i]);

        my_strcpy(mount->root->d_subdirs[i]->d_name, file_arr[i].path_name);
        mount->root->d_subdirs[i]->d_parent = mount->root;
        for (int j = 0; j < 16; j++)
            mount->root->d_subdirs[i]->d_subdirs[j] = NULL;
    }

    mount->fs = fs;

    return 1;
}

int initramfs_write(struct file *file, const void *buf, size_t len)
{
    uart_puts("you can't write in initramfs!\n");
    return 1;
}

int initramfs_read(struct file *file, void *buf, size_t len)
{
    FILE* initramfs_internal = (struct FILE *)file->f_dentry->d_inode->internal;

    char *dest = (char *)buf;
    char *src = &initramfs_internal->file_content[file->f_pos];
    int i = 0;
    for (; i < len && src[i] != (unsigned char)EOF; i++)
        dest[i] = src[i];
    dest[i] = '\0';
    
    file->f_pos += i;

    return i;
}

struct dentry *initramfs_lookup(struct inode *dir, const char *component_name)
{
    struct dentry *cur = dir->i_dentry;
    for (int i = 0; i < 16; i++)
    {
        if (cur->d_subdirs[i] != NULL && my_strcmp(cur->d_subdirs[i]->d_name, component_name) == 0)
            return cur->d_subdirs[i];
    }

    return NULL;
}

int initramfs_create(struct inode *dir, struct dentry *dentry, int mode)
{
    uart_puts("you can't create in initramfs!\n");
    return 1;
}

int initramfs_mkdir(struct inode *dir, struct dentry *dentry)
{
    uart_puts("you can't mkdir in initramfs!\n");
    return 1;
}

void traverse_file()
{
    for (int i = 0; i < file_num; i++)
    {
        uart_puts(file_arr[i].path_name);
        uart_puts("\n");
    }
}

void look_file_content(char *pathname)
{
    for (int i = 0; i < file_num; i++)
    {
        if (my_strcmp(file_arr[i].path_name, pathname) == 0)
        {
            uart_puts(file_arr[i].file_content);
            uart_puts("\n");
        }
    }
}