#pragma once

#include "fs/fwd.hpp"

extern Vnode* root_node;
void init_vfs();

extern FileSystem* filesystems;
FileSystem** find_filesystem(const char* name);
FileSystem* get_filesystem(const char* name);
int register_filesystem(FileSystem* fs);

int open(const char* pathname, fcntl flags);
int close(int fd);
long write(int fd, const void* buf, unsigned long count);
long read(int fd, void* buf, unsigned long count);

int vfs_open(const char* pathname, fcntl flags, File*& target);
int vfs_close(File* file);
int vfs_write(File* file, const void* buf, size_t len);
int vfs_read(File* file, void* buf, size_t len);
int vfs_mkdir(const char* pathname);
int vfs_mount(const char* target, const char* filesystem);
Vnode* vfs_lookup(const char* pathname);
int vfs_lookup(const char* pathname, Vnode*& target);
int vfs_lookup(Vnode* base, const char* pathname, Vnode*& target);
int vfs_lookup(const char* pathname, Vnode*& target, char*& basename);
int vfs_lookup(Vnode* base, const char* pathname, Vnode*& target,
               char*& basename);
