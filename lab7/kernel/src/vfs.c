#include "utils.h"
#include "allocator.h"
#include "tmpfs.h"
#include "uart.h"
#include "schedule.h"
#include "vfs.h"

struct filesystem *file_systems;
struct mount *rootfs;

static struct filesystem tmpfs_type = {
	.name = "tmpfs",
	.setup_mount = tmpfs_setup_mount,
	.next = NULL};

int register_filesystem(struct filesystem *fs)
{
	// register the file system to the kernel.
	// you can also initialize memory pool of the file system here.

	for (struct filesystem *cur = file_systems; cur != NULL; cur = cur->next) // find whether fs is registered before.
		if (my_strcmp(cur->name, fs->name) == 0)
			return 1;

	struct filesystem *cur = file_systems; // insert fs to file_systems list.
	if (cur == NULL)
		file_systems = fs;
	else
	{
		while (cur->next != NULL)
			cur = cur->next;
		cur->next = fs;
	}

	return 1;
}

struct file *vfs_open(const char *pathname, int flags)
{
	struct file *fd = NULL;
	char file_name[32];
	struct dentry *dir = vfs_lookup(pathname, file_name); // Lookup pathname.
	struct dentry *f_inode = dir->d_inode->i_ops->lookup(dir->d_inode, file_name);

	if (f_inode) // Create a new file descriptor for this vnode if found.
	{
		fd = kmalloc(sizeof(struct file));
		fd->f_dentry = f_inode;
		fd->f_ops = f_inode->d_inode->f_ops;
		fd->f_pos = 0;
		fd->flags = flags;
	}
	else if (flags & O_CREAT) // Create a new file if O_CREAT is specified in flags.
	{
		fd = kmalloc(sizeof(struct file));
		fd->f_dentry = kmalloc(sizeof(struct dentry));

		my_strcpy(fd->f_dentry->d_name, file_name);
		fd->f_dentry->d_parent = dir;
		for (int i = 0; i < 16; i++)
			fd->f_dentry->d_subdirs[i] = NULL;

		dir->d_inode->i_ops->create(dir->d_inode, fd->f_dentry, flags & ~(O_CREAT));

		fd->f_ops = fd->f_dentry->d_inode->f_ops;
		fd->f_pos = 0;
		fd->flags = flags & ~(O_CREAT);
	}

	return fd;
}

int vfs_close(struct file *file)
{
	// release the file descriptor
	kfree(file);
	return 1;
}

int vfs_write(struct file *file, const void *buf, size_t len)
{
	// write len byte from buf to the opened file.
	int size = file->f_ops->write(file, buf, len);
	// return written size or error code if an error occurs.
	return size;
}

int vfs_read(struct file *file, void *buf, size_t len)
{
	// read min(len, readable file data size) byte to buf from the opened file.
	int size = file->f_ops->read(file, buf, len);
	// return read size or error code if an error occurs.
	return size;
}

int vfs_mkdir(const char *pathname)
{
	char subdir_name[32];
	struct dentry *dir = vfs_lookup(pathname, subdir_name); // Lookup pathname.
	for (int i = 0; i < 16; i++)
		if (dir->d_subdirs[i] != NULL && my_strcmp(dir->d_subdirs[i]->d_name, subdir_name) == 0)
		{
			uart_puts("folder existes\n");
			return -1;
		}

	struct dentry *f_dentry = kmalloc(sizeof(struct dentry));

	my_strcpy(f_dentry->d_name, subdir_name);
	f_dentry->d_parent = dir;
	for (int i = 0; i < 16; i++)
		f_dentry->d_subdirs[i] = NULL;

	dir->d_inode->i_ops->mkdir(dir->d_inode, f_dentry);

	return 1;
}

int vfs_chdir(const char *pathname)
{
	struct dentry *cur_dir = NULL;

	if (pathname[0] == '/')
	{
		pathname += 1;
		cur_dir = rootfs->root;
	}
	else if (pathname[0] == '.' && pathname[1] == '/')
	{
		pathname += 2;
		cur_dir = get_current_task()->pwd;
	}
	else if (pathname[0] == '.' && pathname[1] == '.' && pathname[2] == '/')
	{
		pathname += 3;
		cur_dir = get_current_task()->pwd->d_parent;
	}
	else
	{
		cur_dir = get_current_task()->pwd;
	}

	char component_name[32][32];
	const char *cur_component = pathname;
	int component_idx = 0;

	int c_idx = 0;
	while (1)
	{
		if (*cur_component == '/')
		{
			component_name[component_idx][c_idx] = '\0';
			component_idx++;
			c_idx = 0;
			cur_component += 1;
			continue;
		}
		else if (*cur_component == '\0')
		{
			component_name[component_idx][c_idx] = '\0';
			break;
		}

		component_name[component_idx][c_idx++] = *cur_component;
		cur_component += 1;
	}

	for (int i = 0; i <= component_idx; i++)
	{
		if (my_strcmp(component_name[i], ".") == 0)
			cur_dir = cur_dir;
		else if (my_strcmp(component_name[i], "..") == 0)
			cur_dir = cur_dir->d_parent;
		else
			cur_dir = cur_dir->d_inode->i_ops->lookup(cur_dir->d_inode, component_name[i]);

		if (cur_dir == NULL)
			return -1;
	}

	get_current_task()->pwd = cur_dir;

	return 1;
}

int vfs_mount(const char *device, const char *mountpoint, const char *filesystem)
{
	struct filesystem *fs = file_systems;
	for (; fs != NULL; fs = fs->next) // find the filesystem that want to mount
	{
		if (my_strcmp(fs->name, filesystem) == 0)
			break;
	}

	struct mount *new_mount = kmalloc(sizeof(struct mount)); // create a new filesysteam
	fs->setup_mount(fs, new_mount);

	char subdir_name[32];
	struct dentry *dir = vfs_lookup(mountpoint, subdir_name); // Lookup pathname.
	for (int i = 0; i < 16; i++)							  // mount the new filesystem on mountpoint
	{
		if (my_strcmp(dir->d_subdirs[i]->d_name, subdir_name) == 0)
		{
			dir->d_subdirs[i] = new_mount->root;
			new_mount->root->d_parent = dir;
			my_strcpy(new_mount->root->d_name, subdir_name);
			break;
		}
	}

	return 1;
}

struct dentry *vfs_lookup(const char *pathname, char *file_name)
{
	struct dentry *cur_dir = NULL;

	if (pathname[0] == '/')
	{
		pathname += 1;
		cur_dir = rootfs->root;
	}
	else if (pathname[0] == '.' && pathname[1] == '/')
	{
		pathname += 2;
		cur_dir = get_current_task()->pwd;
	}
	else if (pathname[0] == '.' && pathname[1] == '.' && pathname[2] == '/')
	{
		pathname += 3;
		cur_dir = get_current_task()->pwd->d_parent;
	}
	else
	{
		cur_dir = get_current_task()->pwd;
	}

	char component_name[32][32];
	const char *cur_component = pathname;
	int component_idx = 0;

	int c_idx = 0;
	while (1)
	{
		if (*cur_component == '/')
		{
			component_name[component_idx][c_idx] = '\0';
			component_idx++;
			c_idx = 0;
			cur_component += 1;
			continue;
		}
		else if (*cur_component == '\0')
		{
			component_name[component_idx][c_idx] = '\0';
			my_strcpy(file_name, component_name[component_idx]);
			break;
		}

		component_name[component_idx][c_idx++] = *cur_component;
		cur_component += 1;
	}

	for (int i = 0; i < component_idx; i++)
	{
		if (my_strcmp(component_name[i], ".") == 0)
			cur_dir = cur_dir;
		else if (my_strcmp(component_name[i], "..") == 0)
			cur_dir = cur_dir->d_parent;
		else
			cur_dir = cur_dir->d_inode->i_ops->lookup(cur_dir->d_inode, component_name[i]);

		if (cur_dir == NULL)
			return NULL;
	}

	return cur_dir;
}

void rootfs_init()
{
	register_filesystem(&tmpfs_type);

	struct filesystem *cur = file_systems;
	for (; cur != NULL; cur = cur->next) // find tmpfs
		if (my_strcmp(cur->name, "tmpfs") == 0)
			break;

	rootfs = kmalloc(sizeof(struct mount));
	cur->setup_mount(cur, rootfs);
	rootfs->root->d_parent = rootfs->root;

	get_current_task()->pwd = rootfs->root;
}