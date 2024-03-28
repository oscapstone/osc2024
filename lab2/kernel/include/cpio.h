#ifndef _CPIO_H_
#define _CPIO_H_

/*
    cpio format : https://manpages.ubuntu.com/manpages/bionic/en/man5/cpio.5.html
    We are using "newc" format
    header, file path, file data, header  ......
    header + file path (padding 4 bytes)
    file data (padding 4 bytes)  (max size 4gb)
*/

#define CPIO_NEWC_HEADER_MAGIC "070701"    // big endian constant, to check whether it is big endian or little endian

// Using newc archive format
struct cpio_newc_header
{
    char c_magic[6];      /* Magic header '070701'. */
    char c_ino[8];        /* "i-node" number of the file. In traditional Unix filesystems, this is a unique identifier for a file or directory*/
    char c_mode[8];       /* Permisions. The file mode, which specifies the file type and permissions (e.g., read, write, execute) in a bitmask format.*/
    char c_uid[8];        /* User ID of the file's owner. */
    char c_gid[8];        /* Group ID of the file's owner. */
    char c_nlink[8];      /* The number of hard links to this file. For a regular file, this is typically 1. For a directory, it is the number of subdirectories (including the parent directory "." and the parent's parent directory ".."). */
    char c_mtime[8];      /* The modification time of the file, typically represented as the number of seconds since the Unix epoch (January 1, 1970). */
    char c_filesize[8];   /* File size in bytes. For a directory, this is usually zero. */
    char c_devmajor[8];   /* Major dev number. 在Unix 系統中，每個裝置檔案都與一個裝置號碼相關聯，該裝置號碼由主裝置號碼和次裝置號碼組成。主設備號碼用於識別設備類型，如硬碟、終端機或網路設備等，而次設備號碼用於區分同一類型中的不同設備。*/
    char c_devminor[8];   /* Minor dev number. These are used for special files (e.g., device nodes) to indicate which device the file refers to.*/
    char c_rdevmajor[8];  /* 這兩個欄位與c_devmajor和c_devminor類似，但它們用於表示特殊設備檔案本身的設備號。在Unix 系統中，特殊設備檔案（如字元設備檔案和區塊設備檔案）用於提供對硬體設備的存取。這些欄位儲存了特殊設備檔案所代表的實際硬體設備的主設備號碼和次設備號碼。*/
    char c_rdevminor[8];  /* The major and minor device numbers for the device that this file represents, if it is a special file (e.g., a device node).*/
    char c_namesize[8];   /* Length of filename in bytes, including the null terminator. */
    char c_check[8];      /* Checksum. This is not widely used and is often set to zero.*/
};

/* write pathname, data, next header into corresponding parameter*/
int cpio_newc_parse_header(struct cpio_newc_header *this_header_pointer,
        char **pathname, unsigned int *filesize, char **data,
        struct cpio_newc_header **next_header_pointer);

#endif /* _CPIO_H_ */
