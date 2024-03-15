/*
 * Code modified from SEL4PROJ@github (https://github.com/SEL4PROJ/libcpio)
 */

/*
 * Parse cpio archive files in "New ASCII Format"
 * 
 * Freebsd’s man page (https://www.freebsd.org/cgi/man.cgi?query=cpio&sektion=5)
 * has a detailed definition of how New ASCII Format Cpio Archive is structured.
 */

#ifndef _LIB_CPIO_H_
#define _LIB_CPIO_H_

/* Magic identifiers for the "cpio" file format. */
#define CPIO_HEADER_MAGIC "070701"
#define CPIO_FOOTER_MAGIC "TRAILER!!!"
#define CPIO_ALIGNMENT 4

#define INITRAMFS_ADDR 0x20000000

#ifndef NULL
#define NULL ((void *)0)
#endif

/* CPIO "New ASCII Format" header structure */
/* Header -> Filename -> Data ...... "TRAILER!!!" */
struct cpio_header {
    char    c_magic[6];         // 魔数 (magic string) - 固定为 "070701"
    char    c_ino[8];           // 文件索引节点号 (inode number)
    char    c_mode[8];          // 文件权限 (file mode)
    char    c_uid[8];           // 用户 ID (user ID)
    char    c_gid[8];           // 组 ID (group ID)
    char    c_nlink[8];         // 硬链接数 (number of hard links)
    char    c_mtime[8];         // 最后修改时间 (last modification time)
    char    c_filesize[8];      // 文件大小 (file size)
    char    c_devmajor[8];      // 设备主要编号 (device major number)
    char    c_devminor[8];      // 设备次要编号 (device minor number)
    char    c_rdevmajor[8];     // 特殊设备主要编号 (special device major number)
    char    c_rdevminor[8];     // 特殊设备次要编号 (special device minor number)
    char    c_namesize[8];      // 路径名长度 (pathname size)
    char    c_check[8];         // 校验和 (checksum) - 总是被设置为 0
};


/**
 * 
 * Return value define for cpio_parse_header()
 * 
 */
 // TODO:
 // Embed this object to cpio_parse_header function
enum cpio_parse_header_result
{
    notValid = -1,
    OK = 0,
    EOF = 1
};

/**
 * Stores information about the underlying implementation.
 */
struct cpio_info {
    /// The number of files in the CPIO archive
    unsigned int file_count;
    /// The maximum size of a file name
    unsigned int max_path_sz;
};

/**
 * Parse the header of the given CPIO entry.
 * Return -1 if the header is not valid, 1 if it is EOF.
 *
 * @param[in] archive     The location of the CPIO archive
 * @param[out] filename   ###The name of the file in question.
 * @param[out] _filesize  ###The name of the file in question.
 * @param[out] data       ###The name of the file in question.
 * @param[out] next       ####The name of the file in question.
 */
int cpio_parse_header(struct cpio_header *archive,
        const char **filename, unsigned long *_filesize, void **data,
        struct cpio_header **next);

/**
 * Retrieve file information from a provided file name
 * @param[in] archive  The location of the CPIO archive
 * @param[in] name     The name of the file in question.
 * @param[out] size    The retrieved size of the file in question
 * @return             The location of the file in memory; NULL if the file
 *                     does not exist.
 */
void *cpio_get_file(void *archive, const char *name, unsigned long *size);

/**
 * Writes the list of file names contained within a CPIO archive into 
 * a provided buffer
 * @param[in] archive  The location of the CPIO archive
 * @param[in] buf      A memory location to store the CPIO file list to
 * @param[in] buf_len  The length of the provided buf
 */
void cpio_ls(void *archive);

// Callback function of dts
int initrd_fdt_callback(void *, int);
int initrd_getLo(void);


#endif /* _LIB_CPIO_H_ */
