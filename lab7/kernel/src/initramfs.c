#include "initramfs.h"
#include "vfs.h"
#include "string.h"
#include "memory.h"
#include "uart1.h"
#include "stddef.h"
#include "string.h"
#include "list.h"
#include "debug.h"
#include "cpio.h"

const file_operations_t initramfs_file_operations = {initramfs_write, initramfs_read, initramfs_open, initramfs_close, initramfs_lseek64, initramfs_getsize};
const vnode_operations_t initramfs_vnode_operations = {initramfs_lookup, initramfs_create, initramfs_mkdir, initramfs_readdir};
filesystem_t *initramfs_fs;
extern char *CPIO_START;
extern char *CPIO_END;

int register_initramfs()
{
	initramfs_fs = kmalloc(sizeof(filesystem_t));
	initramfs_fs->name = "initramfs";
	initramfs_fs->setup_mount = initramfs_setup_mount;
	register_filesystem(initramfs_fs);
	return 0;
}

static inline size_t get_node_name(char *c_filepath, size_t start_index, size_t *end_index, char buf[])
{
	size_t i = start_index;
	enum fsnode_type type;
	while (c_filepath[i] != '\0' && c_filepath[i] != '/')
	{
		buf[i - start_index] = c_filepath[i];
		i++;
	}
	*end_index = i;

	if (i == start_index)
	{
		return -1;
	}
	buf[i - start_index] = '\0';

	if (c_filepath[i] == '/')
	{
		type = FS_DIR;
	}
	else if (c_filepath[i] == '\0')
	{
		type = FS_FILE;
	}
	else
	{
		ERROR("get_node_name: unknown type\r\n");
		return -1;
	}
	DEBUG("get_node_name: %s, type: %d, i: %d, c_filepath[i]: %c\r\n", buf, type, i, c_filepath[i]);
	return type;
}

static inline void __gen_vnode_of_initramfs(mount_t *_mount)
{
	char compoent_name[MAX_FILE_NAME];
	char *c_filepath;
	char *c_filedata;
	unsigned int c_filesize;
	int error;

	size_t start_index = 0;
	size_t end_index = 0;
	size_t len = 0;
	vnode_t *curr_node;
	vnode_t *n;
	enum fsnode_type type = 0;
	int end = 1;
	CPIO_FOR_EACH(&c_filepath, &c_filesize, &c_filedata, error, {
		curr_node = _mount->root;
		start_index = 0;
		len = strlen(c_filepath);
		end = 1;
		while (end)
		{
			DEBUG("start_index: %d, len: %d\r\n", start_index, len);
			type = get_node_name(c_filepath, start_index, &end_index, compoent_name);
			start_index = end_index + 1;
			DEBUG("start_index: %d, end_index: %d, len: %d\r\n", start_index, end_index, len);
			if (end_index == len)
			{
				if (c_filesize != 0)
					type = FS_FILE;
				else
					type = FS_DIR;
				end = 0;
			}
			if (type == -1)
			{
				break;
			}
			DEBUG("initramfs_setup_mount: c_filepath: %s, compoent_name: %s, strlen: %d, type: %d\r\n", c_filepath, compoent_name, strlen(compoent_name), type);
			if (vfs_lookup(curr_node, compoent_name, &curr_node) == 0) // found
			{
				DEBUG("found\r\n");
				continue;
			}
			DEBUG("not found\r\n");
			if (type == FS_DIR)
			{
				DEBUG("mkdir %s\r\n", compoent_name);

				curr_node = __initramfs_mkdir(curr_node, &n, compoent_name);
			}
			else if (type == FS_FILE)
			{
				DEBUG("create\r\n");
				initramfs_create_vnode(_mount, FS_FILE, curr_node, compoent_name, c_filedata, c_filesize);
			}
		}
	});
}

/**
 * @brief Setup the superblock point for initramfs
 *
 * @param fs
 * @param _mount
 * @param name superblock point name
 */
int initramfs_setup_mount(filesystem_t *fs, mount_t *_mount, vnode_t *parent, const char *name)
{
	_mount->fs = fs;
	_mount->v_ops = &initramfs_vnode_operations;
	_mount->f_ops = &initramfs_file_operations;
	_mount->root = initramfs_create_vnode(_mount, FS_DIR, NULL, name, NULL, 0);
	_mount->root->parent = parent;
	__gen_vnode_of_initramfs(_mount);

	return 0;
}

vnode_t *initramfs_create_vnode(mount_t *superblock, enum fsnode_type type, vnode_t *parent, const char *name, char *data, size_t datasize)
{
	vnode_t *node = create_vnode();
	node->superblock = superblock;
	node->v_ops = &initramfs_vnode_operations;
	node->f_ops = &initramfs_file_operations;
	node->parent = parent;
	node->mount = NULL;
	node->type = type;
	initramfs_inode_t *inode = initramfs_create_inode(type, name, data, datasize);
	node->internal = inode;
	node->name = inode->name;

	if (parent != NULL)
	{
		initramfs_inode_t *dir_inode = (initramfs_inode_t *)parent->internal;
		vnode_list_t *newvnode_list = kmalloc(sizeof(vnode_list_t));
		newvnode_list->vnode = node;
		list_add_tail((list_head_t *)newvnode_list, (list_head_t *)dir_inode->child_list);
	}
	DEBUG("node name: %s\r\n", node->name);
	DEBUG("inode name: %s\r\n", inode->name);
	return node;
}

initramfs_inode_t *initramfs_create_inode(enum fsnode_type type, const char *name, char *data, size_t datasize)
{
	struct initramfs_inode *inode = kmalloc(sizeof(struct initramfs_inode));
	inode->data = data;
	inode->datasize = datasize;
	DEBUG("initramfs_create_inode: %s\r\n", name);
	strcpy(inode->name, name);
	DEBUG("initramfs_create_inode: inode->name %s\r\n", inode->name);
	if (type == FS_DIR)
	{
		inode->child_list = kmalloc(sizeof(vnode_list_t));
		INIT_LIST_HEAD(&inode->child_list->list_head);
	}
	else if (type == FS_FILE)
	{
		inode->child_list = NULL;
	}
	else
	{
		ERROR("initramfs_create_inode: unknown type\r\n");
		return NULL;
	}
	return inode;
}

// file operations
int initramfs_write(struct file *file, const void *buf, size_t len)
{
	ERROR("initramfs can't call write\r\n");
	return -1;
}

int initramfs_read(struct file *file, void *buf, size_t len)
{
	struct initramfs_inode *inode = file->vnode->internal;
	// if buffer overflow, shrink the request read length
	// read from f_pos
	if (len + file->f_pos > inode->datasize)
	{
		len = inode->datasize - file->f_pos;
		memcpy(buf, inode->data + file->f_pos, len);
		file->f_pos += inode->datasize - file->f_pos;
		return len;
	}
	else
	{
		memcpy(buf, inode->data + file->f_pos, len);
		file->f_pos += len;
		return len;
	}
	return -1;
}

int initramfs_open(struct vnode *file_node, struct file **target)
{
	(*target)->vnode = file_node;
	(*target)->f_ops = file_node->f_ops;
	(*target)->f_pos = 0;
	return 0;
}

int initramfs_close(struct file *file)
{
	kfree(file);
	return 0;
}

long initramfs_lseek64(struct file *file, long offset, int whence)
{
	if (whence == SEEK_SET)
	{
		file->f_pos = offset;
		return file->f_pos;
	}
	return -1;
}

long initramfs_getsize(struct vnode *vd)
{
	struct initramfs_inode *inode = vd->internal;
	return inode->datasize;
}

// vnode operations
int initramfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
	DEBUG("initramfs_lookup: %s\r\n", dir_node->name);
	initramfs_inode_t *dir_inode = (initramfs_inode_t *)dir_node->internal;
	list_head_t *curr;
	vnode_t *child_vnode;
	initramfs_inode_t *child_inode;
	list_for_each(curr, (list_head_t *)(dir_inode->child_list))
	{
		child_vnode = ((vnode_list_t *)curr)->vnode;
		child_inode = (initramfs_inode_t *)(child_vnode->internal);
		if (strcmp(child_vnode->name, component_name) == 0)
		{
			*target = child_vnode;
			return 0;
		}
	}
	return -1;
}

int initramfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
	ERROR("initramfs can't call create\r\n");
	return -1;
}

int initramfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
	ERROR("initramfs can't call mkdir\r\n");
	return -1;
}

int __initramfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
	struct initramfs_inode *inode = dir_node->internal;

	if (dir_node->type != FS_DIR)
	{
		ERROR("initramfs mkdir not dir_t\r\n");
		return -1;
	}

	if (strlen(component_name) > MAX_FILE_NAME)
	{
		ERROR("FILE NAME TOO LONG\r\n");
		return -1;
	}

	DEBUG("initramfs_mkdir: %s\r\n", dir_node->name);
	struct vnode *_vnode = initramfs_create_vnode(dir_node->superblock, FS_DIR, dir_node, component_name, NULL, 0);
	struct initramfs_inode *newinode = _vnode->internal;

	*target = _vnode;
	return 0;
}

int initramfs_readdir(struct vnode *dir_node, const char name_array[])
{
	struct initramfs_inode *inode = dir_node->internal;
	DEBUG("initramfs_readdir: %s\r\n", dir_node->name);

	if (dir_node->type != FS_DIR)
	{
		ERROR("initramfs readdir not dir_t\r\n");
		return -1;
	}

	list_head_t *curr;
	vnode_t *child_vnode;
	size_t max_len = 0;
	char *name_array_start = name_array;
	list_for_each(curr, (list_head_t *)(inode->child_list))
	{
		child_vnode = ((vnode_list_t *)curr)->vnode;
		DEBUG("initramfs_readdir: child_vnode->name %s\r\n", child_vnode->name);
		*name_array_start = child_vnode->type;
		strcpy(++name_array_start, child_vnode->name);
		name_array_start += strlen(child_vnode->name) + 1;
	}

	return 0;
}