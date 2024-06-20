#ifndef _CPIO_H_
#define _CPIO_H_

/*
    cpio format : https://manpages.ubuntu.com/manpages/bionic/en/man5/cpio.5.html
    We are using "newc" format
    header, file path, file data, header  ......
    header + file path (padding 4 bytes)
    file data (padding 4 bytes)  (max size 4gb)
*/

// big endian constant, to check whether it is big endian or little endian
#define CPIO_NEWC_HEADER_MAGIC "070701"

typedef enum
{
    CPIO_TRAILER = 0,
    CPIO_SUCCESS = 1,
    CPIO_ERROR = -1
} CPIO_return_t;

/**
 * list_for_each	-	iterate over a list
 * @pos: cpio_newc_header * to use as a loop cursor.
 * @c_filepath:	char **, every time it will be updated to the current file path
 * @c_filesize:	unsigned int *, every time it will be updated to the current file size
 * @c_filedata:	char **, every time it will be updated to the current file data
 * @error:	int, every time it will be updated to the current error code
 * @something_to_run_in_loop: code block that will be executed in every loop
 */
#define CPIO_FOR_EACH(c_filepath, c_filesize, c_filedata, error, something_to_run_in_loop) \
    do                                                                                     \
    {                                                                                      \
        struct cpio_newc_header *pos = (struct cpio_newc_header *)CPIO_START;              \
        while (1)                                                                          \
        {                                                                                  \
            error = cpio_newc_parse_header(pos, c_filepath, c_filesize, c_filedata, &pos); \
            if (error == CPIO_ERROR)                                                       \
            {                                                                              \
                break;                                                                     \
            }                                                                              \
            else if (pos == 0)                                                             \
            {                                                                              \
                error = CPIO_TRAILER;                                                      \
                break; /*if this is TRAILER!!! (last of file)*/                            \
            }                                                                              \
            something_to_run_in_loop                                                       \
        }                                                                                  \
    } while (0)

// The new ASCII format is limited to 4 gigabyte file sizes.
struct cpio_newc_header
{
    char c_magic[6];     // fixed, "070701", check this archive is little-endian or big-endian
    char c_ino[8];       // Inode number
    char c_mode[8];      // Specifies permissions / file type
    char c_uid[8];       // Unique ID
    char c_gid[8];       // Group ID
    char c_nlink[8];     // The number of links associated with this file
    char c_mtime[8];     // File's last modification timestamp
    char c_filesize[8];  // File size
    
    /* The major / minor number of the device containing the file system from which the file was read. */
    char c_devmajor[8];
    char c_devminor[8];

    /* The major number of the raw device containing the file system from which the file was read. */
    char c_rdevmajor[8]; 
    char c_rdevminor[8]; 

    char c_namesize[8];  // The length (byte) of filename located following the header (including NULL)
    char c_check[8];     // Always set to zero by writers
};

// Parse header and write into related parameter
int cpio_parse_header(struct cpio_newc_header *header_ptr, char **path, 
    unsigned int *filesize, char **data, struct cpio_newc_header **next_header_ptr);

CPIO_return_t cpio_newc_parse_header(struct cpio_newc_header *this_header_pointer,
     char **pathname, unsigned int *filesize, char **data, struct cpio_newc_header **next_header_pointer);

unsigned int    padding_4byte(unsigned int size);
int             cpio_get_file(const char *filepath, unsigned int *c_filesize, char **c_filedata);


#endif /* _CPIO_H_ */