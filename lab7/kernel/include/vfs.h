#ifndef __VFS_H
#define __VFS_H

#define size_t unsigned long long
#define EOF -1

#define O_CREAT 00000100

struct inode
{
	struct inode_operations *i_ops;
	struct file_operations *f_ops;
	struct dentry *i_dentry; // the dentry that pointer to this inode
	void *internal;			 // the filesysteam's internal data
};

struct dentry
{
	char d_name[32];			  // the name of directory or file
	struct dentry *d_parent;	  // the parent of this dentry
	struct inode *d_inode;		  // the inode which store the metadata of directory of file
	struct dentry *d_subdirs[16]; // the sub-directories of this directory
};

struct file
{
	struct dentry *f_dentry;
	size_t f_pos; // The next read/write position of this file descriptor
	struct file_operations *f_ops;
	int flags;
};

struct mount
{
	struct dentry *root;
	struct filesystem *fs;
};

struct filesystem
{
	const char *name;
	int (*setup_mount)(struct filesystem *fs, struct mount *mount);
	struct filesystem *next;
};

struct file_operations
{
	int (*write)(struct file *file, const void *buf, size_t len);
	int (*read)(struct file *file, void *buf, size_t len);
};

struct inode_operations
{
	struct dentry *(*lookup)(struct inode *dir, const char *component_name);
	int (*create)(struct inode *dir, struct dentry *dentry, int mode);
	int (*mkdir)(struct inode *dir, struct dentry *dentry);
};

extern struct filesystem *file_systems;
extern struct mount *rootfs;

int register_filesystem(struct filesystem *fs);
struct file *vfs_open(const char *pathname, int flags);
int vfs_close(struct file *file);
int vfs_write(struct file *file, const void *buf, size_t len);
int vfs_read(struct file *file, void *buf, size_t len);

int vfs_mkdir(const char *pathname);
int vfs_chdir(const char *pathname);
int vfs_mount(const char *device, const char *mountpoint, const char *filesystem);
struct dentry *vfs_lookup(const char *pathname, char *file_name);

void rootfs_init();

#endif