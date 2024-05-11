#ifndef INITRD_H
#define INITRD_H

#define CPIO_BASE_QEMU  (0x8000000)
#define CPIO_BASE_RPI   (0x20000000)

#define CPIO_NEWC_HEADER_MAGIC "070701"    // big endian


extern char *ramfs_base;
extern char *ramfs_end;

// Cpio Archive File Header (New ASCII Format)
typedef struct {
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
} cpio_t;

int cpio_newc_parse_header(cpio_t *this_header_pointer, char **pathname, unsigned int *filesize, char **data, cpio_t **next_header_pointer);
void initrd_list();
void initrd_cat(const char *target);
void initrd_callback(unsigned int node_type, char *name, void *value, unsigned int name_size);
void initrd_exec_prog(char* target);

#endif // INITRD_H