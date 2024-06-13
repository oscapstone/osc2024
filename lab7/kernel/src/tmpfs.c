#include "tmpfs.h"
#include "vfs.h"
#include "string.h"
#include "memory.h"
#include "uart1.h"
#include "stddef.h"
#include "string.h"
#include "list.h"
#include "debug.h"

file_operations_t tmpfs_file_operations = {tmpfs_write, tmpfs_read, tmpfs_open, tmpfs_close, tmpfs_lseek64, tmpfs_getsize};
vnode_operations_t tmpfs_vnode_operations = {tmpfs_lookup, tmpfs_create, tmpfs_mkdir, tmpfs_readdir};
filesystem_t *tmpfs_fs;

int register_tmpfs()
{
	tmpfs_fs = kmalloc(sizeof(filesystem_t));
	tmpfs_fs->name = "tmpfs";
	tmpfs_fs->setup_mount = tmpfs_setup_mount;
	register_filesystem(tmpfs_fs);
	return 0;
}

/**
 * @brief Setup the superblock point for tmpfs
 *
 * @param fs
 * @param _mount
 * @param name superblock point name
 */
int tmpfs_setup_mount(filesystem_t *fs, mount_t *_mount, vnode_t *parent, const char *name)
{
	_mount->fs = fs;
	_mount->v_ops = &tmpfs_vnode_operations;
	_mount->f_ops = &tmpfs_file_operations;
	_mount->root = tmpfs_create_vnode(_mount, FS_DIR, parent, name);
	return 0;
}

vnode_t *tmpfs_create_vnode(mount_t *superblock, enum fsnode_type type, vnode_t *parent, const char *name)
{
	vnode_t *node = create_vnode();
	node->superblock = superblock;
	node->superblock->v_ops = &tmpfs_vnode_operations;
	node->superblock->f_ops = &tmpfs_file_operations;
	node->parent = parent;
	node->mount = NULL;
	node->type = type;
	tmpfs_inode_t *inode = tmpfs_create_inode(type, name);
	node->internal = inode;
	node->name = inode->name;

	if (parent != NULL)
	{
		tmpfs_inode_t *dir_inode = (tmpfs_inode_t *)parent->internal;
		vnode_list_t *newvnode_list = kmalloc(sizeof(vnode_list_t));
		newvnode_list->vnode = node;
		list_add_tail((list_head_t *)newvnode_list, (list_head_t *)dir_inode->child_list);
	}
	DEBUG("node name: %s\r\n", node->name);
	DEBUG("inode name: %s\r\n", inode->name);
	return node;
}

tmpfs_inode_t *tmpfs_create_inode(enum fsnode_type type, const char *name)
{
	struct tmpfs_inode *inode = kmalloc(sizeof(struct tmpfs_inode));
	inode->data = NULL;
	inode->datasize = 0;
	DEBUG("tmpfs_create_inode: %s\r\n", name);
	strcpy(inode->name, name);
	DEBUG("tmpfs_create_inode: inode->name %s\r\n", inode->name);
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
		ERROR("tmpfs_create_inode: unknown type\r\n");
		return NULL;
	}
	return inode;
}

// file operations
int tmpfs_write(struct file *file, const void *buf, size_t len)
{
	struct tmpfs_inode *inode = file->vnode->internal;
	// write from f_pos
	memcpy(inode->data + file->f_pos, buf, len);
	// update f_pos and size
	file->f_pos += len;
	if (inode->datasize < file->f_pos)
		inode->datasize = file->f_pos;
	return len;
}

int tmpfs_read(struct file *file, void *buf, size_t len)
{
	struct tmpfs_inode *inode = file->vnode->internal;
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

int tmpfs_open(struct vnode *file_node, struct file **target)
{
	(*target)->vnode = file_node;
	(*target)->f_ops = file_node->superblock->f_ops;
	(*target)->f_pos = 0;
	return 0;
}

int tmpfs_close(struct file *file)
{
	kfree(file);
	return 0;
}

long tmpfs_lseek64(struct file *file, long offset, int whence)
{
	if (whence == SEEK_SET)
	{
		file->f_pos = offset;
		return file->f_pos;
	}
	return -1;
}

long tmpfs_getsize(struct vnode *vd)
{
}

// vnode operations
int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
	tmpfs_inode_t *dir_inode = (tmpfs_inode_t *)dir_node->internal;
	list_head_t *curr;
	vnode_t *child_vnode;
	tmpfs_inode_t *child_inode;
	list_for_each(curr, (list_head_t *)(dir_inode->child_list))
	{
		child_vnode = ((vnode_list_t *)curr)->vnode;
		child_inode = (tmpfs_inode_t *)(child_vnode->internal);
		if (strcmp(child_vnode->name, component_name) == 0)
		{
			*target = child_vnode;
			return 0;
		}
	}
	return -1;
}

int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
	struct tmpfs_inode *inode = dir_node->internal;

	if (dir_node->type != FS_DIR)
	{
		ERROR("tmpfs create not dir_t\r\n");
		return -1;
	}

	if (strlen(component_name) > MAX_FILE_NAME)
	{
		ERROR("FILE NAME TOO LONG\r\n");
		return -1;
	}

	vnode_t *_vnode = tmpfs_create_vnode(dir_node->superblock, FS_FILE, dir_node, component_name);

	*target = _vnode;
	return 0;
}

int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
	struct tmpfs_inode *inode = dir_node->internal;

	if (dir_node->type != FS_DIR)
	{
		ERROR("tmpfs mkdir not dir_t\r\n");
		return -1;
	}

	if (strlen(component_name) > MAX_FILE_NAME)
	{
		ERROR("FILE NAME TOO LONG\r\n");
		return -1;
	}

	struct vnode *_vnode = tmpfs_create_vnode(dir_node->superblock, FS_DIR, dir_node, component_name);
	struct tmpfs_inode *newinode = _vnode->internal;

	*target = _vnode;
	return 0;
}

int tmpfs_readdir(struct vnode *dir_node, const char name_array[])
{
	struct tmpfs_inode *inode = dir_node->internal;
	DEBUG("tmpfs_readdir: %s\r\n", dir_node->name);

	if (dir_node->type != FS_DIR)
	{
		ERROR("tmpfs readdir not dir_t\r\n");
		return -1;
	}

	list_head_t *curr;
	vnode_t *child_vnode;
	size_t max_len = 0;
	char *name_array_start = name_array;
	list_for_each(curr, (list_head_t *)(inode->child_list))
	{
		child_vnode = ((vnode_list_t *)curr)->vnode;
		DEBUG("tmpfs_readdir: child_vnode->name %s, child: 0x%x\r\n", child_vnode->name, child_vnode);
		strcpy(name_array_start, child_vnode->name);
		name_array_start += strlen(child_vnode->name) + 1;
	}

	return 0;
}