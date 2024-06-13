#include "vfs.h"
#include "stddef.h"
#include "stdint.h"
#include "string.h"
#include "list.h"
#include "memory.h"
#include "debug.h"
#include "tmpfs.h"
#include "uart1.h"

vnode_t *root_vnode;
vnode_t *curr_vnode;

mount_t *root_mount;

list_head_t *filesystems_list;

static inline filesystem_t *get_fs(const char *name)
{
	filesystem_t *fs;
	list_head_t *pos;
	list_for_each(pos, (list_head_t *)filesystems_list)
	{
		fs = (filesystem_t *)pos;
		if (strcmp(fs->name, name) == 0)
			return fs;
	}
	return NULL;
}

void init_rootfs()
{
	filesystems_list = kmalloc(sizeof(filesystem_t));
	INIT_LIST_HEAD((list_head_t *)filesystems_list);
	register_tmpfs();
	filesystem_t *fs = get_fs("tmpfs");
	root_mount = kmalloc(sizeof(mount_t));
	fs->setup_mount(fs, root_mount, NULL, "");
	root_vnode = root_mount->root;
	DEBUG("root_vnode: %x\r\n", root_vnode);
	root_vnode->parent = root_vnode;

	curr_vnode = root_vnode;
	vnode_t *node;
	vfs_open("/test", O_CREAT, &node);
	vfs_open("/test1", O_CREAT, &node);
	vfs_open("/test133", O_CREAT, &node);
	vfs_open("/test122", O_CREAT, &node);
	vfs_open("/test11111", O_CREAT, &node);
	vfs_open("/t3t1", O_CREAT, &node);

	vfs_mkdir(curr_vnode, "/dir1");
	vfs_mkdir(curr_vnode, "/dir2");
	vfs_mkdir(curr_vnode, "/dir2/dir3");
	vfs_open("/dir2/test", O_CREAT, &node);

	vfs_mkdir(curr_vnode, "/initramfs");
	register_initramfs();
	vfs_mount("/initramfs", "initramfs");
}

vnode_t *get_root_vnode()
{
	return root_vnode;
}

int handling_relative_path(const char *path, vnode_t *curr_vnode, vnode_t **target, size_t *start_idx)
{
	size_t len = strlen(path);
	DEBUG("handling_relative_path: %s, len: %d, strncmp(path, ., 1): %d\r\n", path, len, strncmp(path, ".", 1));
	if (len != 0)
	{
		if (len >= 3 && strncmp(path, "../", 3) == 0)
		{
			*target = curr_vnode->parent;
			*start_idx = 3;
			return 1;
		}
		else if (len == 2 && strncmp(path, "..", 3) == 0)
		{
			*target = curr_vnode->parent;
			*start_idx = 2;
			return 0;
		}
		else if (len >= 2 && strncmp(path, "./", 2) == 0)
		{
			*target = curr_vnode;
			*start_idx = 2;
			return 1;
		}
		else if (len == 1 && (strncmp(path, ".", 1) == 0 || strncmp(path, "/", 1) == 0))
		{
			*target = curr_vnode;
			*start_idx = 1;
			DEBUG("handling_relative_path: path is ., *target: 0x%x\r\n", *target);
			return 0;
		}
	}
	else
	{
		*target = curr_vnode;
		DEBUG("handling_relative_path: path is empty, *target: 0x%x\r\n", *target);
		return 0;
	}
	*target = curr_vnode;
	return 0;
}

vnode_t *create_vnode()
{
	vnode_t *node = kmalloc(sizeof(vnode_t));
	node->superblock = NULL;
	node->internal = NULL;
	node->name = NULL;
	node->mount = NULL;
	node->parent = NULL;
	return node;
}

int register_filesystem(struct filesystem *fs)
{
	list_add((list_head_t *)fs, (list_head_t *)filesystems_list);
	return 0;
}

int vfs_open(const char *pathname, int flags, struct file **target)
{
	vnode_t *node;
	DEBUG("vfs_open: %s\r\n", pathname);
	if (vfs_lookup(curr_vnode, pathname, &node) == -1)
	{
		if (!(flags & O_CREAT))
		{
			ERROR("cannot find pathname\r\n");
			return -1;
		}
		// grep all of the directory path
		int last_slash_idx = 0;
		for (int i = last_slash_idx; i < strlen(pathname); i++)
		{
			if (pathname[i] == '/')
			{
				last_slash_idx = i;
			}
		}

		char dirname[MAX_PATH_NAME + 1];
		strcpy(dirname, pathname);
		dirname[last_slash_idx] = 0;
		// update dirname to node
		if (vfs_lookup(curr_vnode, dirname, &node) != 0)
		{
			ERROR("cannot ocreate no dir name\r\n");
			return -1;
		}
		// create a new file node on node, &node is new file, 3rd arg is filename
		DEBUG("create file: %s, node: 0x%x\r\n", pathname + last_slash_idx + 1, node);
		node->superblock->v_ops->create(node, &node, pathname + last_slash_idx + 1);
	}
	*target = kmalloc(sizeof(file_t));
	// attach opened file on the new node
	DEBUG("open file %s\r\n", pathname);
	node->superblock->f_ops->open(node, target);
	(*target)->flags = flags;
	return 0;
}

int vfs_close(struct file *file)
{
	// 1. release the file handle
	// 2. Return error code if fails
	file->f_ops->close(file);
	return 0;
}

int vfs_write(struct file *file, const void *buf, size_t len)
{
	// 1. write len byte from buf to the opened file.
	// 2. return written size or error code if an error occurs.
	return file->f_ops->write(file, buf, len);
}

int vfs_read(struct file *file, void *buf, size_t len)
{
	// 1. read min(len, readable size) byte to buf from the opened file.
	// 2. block if nothing to read for FIFO type
	// 2. return read size or error code if an error occurs.
	return file->f_ops->read(file, buf, len);
}

int vfs_readdir(const char *pathname, const char buf[])
{
	DEBUG("vfs_readdir: %s, pathname[0]: %d, pathname[1]: %d\r\n", pathname, pathname[0], pathname[1]);
	struct vnode *node;

	if (vfs_lookup(curr_vnode, pathname, &node) == -1)
	{
		ERROR("vfs_readdir cannot find pathname");
		return -1;
	}
	else
	{
		node->superblock->v_ops->readdir(node, buf);
		return 0;
	}
}

int vfs_mkdir(struct vnode *dir_node, const char *pathname)
{
	char parent_dir_name[MAX_PATH_NAME] = {}; // before add folder
	char new_dir_name[MAX_PATH_NAME] = {};	  // after  add folder

	// search for last directory
	int last_slash_idx = -1;
	for (int i = 0; i < strlen(pathname); i++)
	{
		if (pathname[i] == '/')
		{
			last_slash_idx = i;
		}
	}
	DEBUG("last_slash_idx: %d, dir_node->name: %s\r\n", last_slash_idx, dir_node->name);
	struct vnode *node;
	if (last_slash_idx == -1)
	{
		strcpy(new_dir_name, pathname);
		node = dir_node;
	}
	else
	{
		memcpy(parent_dir_name, pathname, last_slash_idx);
		strcpy(new_dir_name, pathname + last_slash_idx + 1);

		// create new directory if upper directory is found
		if (vfs_lookup(dir_node, parent_dir_name, &node) != 0)
		{
			ERROR("vfs_mkdir cannot find pathname");
			return -1;
		}
	}
	struct vnode *new_node;
	DEBUG("vfs_mkdir: new_dir_name: %s\r\n", new_dir_name);
	node->superblock->v_ops->mkdir(node, &new_node, new_dir_name);
	new_node->type = FS_DIR;
	return 0;
}

int vfs_mount(const char *target, const char *filesystem)
{
	get_fs(filesystem);

	struct vnode *node;
	// search for the target filesystem
	struct filesystem *fs = get_fs(filesystem);
	if (!fs)
	{
		ERROR("vfs_mount cannot find filesystem\r\n");
		return -1;
	}

	if (vfs_lookup(curr_vnode, target, &node) == -1)
	{
		ERROR("vfs_mount cannot find dir\r\n");
		return -1;
	}
	else
	{
		// mount fs on dirnode
		node->mount = kmalloc(sizeof(struct mount));
		fs->setup_mount(fs, node->mount, node->parent, node->name);
	}
	return 0;
}

static inline int __vfs_lookup_depth_1(char *component_name, vnode_t *node, vnode_t **target)
{
	// if fs's v_ops error, return -1
	if (strlen(component_name) != 0) // if component_name is empty, skip
	{
		if (node->superblock->v_ops->lookup(node, &node, component_name) != 0)
		{
			DEBUG("vfs_lookup: lookup error\r\n");
			return -1;
		}
	}
	// redirect to mounted filesystem
	while (node->mount)
	{
		node = node->mount->root;
	}
	*target = node;
	return 0;
}

int vfs_lookup(struct vnode *dir_node, const char *pathname, struct vnode **target)
{
	vnode_t *node;
	size_t i_start = 0;
	// translate absolute path to relative path
	if (pathname[0] == '/')
	{
		DEBUG("vfs_lookup: pathname start with /\r\n");
		node = root_vnode;
		i_start = 1;
	}
	else
	{
		node = dir_node;
	}

	DEBUG("vfs_lookup: pathname %s, str_len: %d\r\n", pathname, strlen(pathname));

	char component_name[MAX_FILE_NAME + 1] = {0};
	size_t offset = 0;
	int comp_len = 0;
	int i = i_start;
	// deal with directory
	do
	{
		DEBUG("vfs_lookup: i: %d, c_idx: %d, pathname[i]: %c\r\n", i, comp_len, pathname[i]);
		while (handling_relative_path(pathname + i, node, &node, &offset)) // if path is relative
		{
			i += offset;
			DEBUG("i: %d, offset: %d\r\n", i, offset);
		}
		i += offset;
		DEBUG("i: %d, offset: %d\r\n", i, offset);
		if (pathname[i - 1] == '/' && pathname[i] == '\0')
			break;
		if (pathname[i] == '/' || pathname[i] == '\0')
		{
			DEBUG("in if pathname[i] == / or \0\r\n");
			component_name[comp_len++] = 0;
			if (__vfs_lookup_depth_1(component_name, node, &node) == -1)
			{
				DEBUG("vfs_lookup: lookup error\r\n");
				return -1;
			}
			comp_len = 0;
		}
		else
		{
			component_name[comp_len++] = pathname[i];
		}
		i++;
	} while (i < strlen(pathname) + 1);
	DEBUG("vfs_lookup: target %s\r\n", node->name);
	*target = node;
	return 0;
}
