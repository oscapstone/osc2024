#ifndef _CPIO_H_
#define _CPIO_H_

/*
    cpio format : https://manpages.ubuntu.com/manpages/bionic/en/man5/cpio.5.html
    We are using "newc" format
    header, file path, file data, header  ......
    header + file path (padding 4 bytes)
    file data (padding 4 bytes)  (max size 4gb)
*/

#define CPIO_NEWC_HEADER_MAGIC "070701" // big endian constant, to check whether it is big endian or little endian
#define TRAILER 0
#define SUCCESS 1
#define ERROR -1

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
    struct cpio_newc_header *pos = CPIO_DEFAULT_PLACE;                                     \
    while (1)                                                                              \
    {                                                                                      \
        error = cpio_newc_parse_header(pos, c_filepath, c_filesize, c_filedata, &pos);     \
        if (pos == 0)                                                                      \
        {                                                                                  \
            error = TRAILER;                                                               \
            break; /*if this is TRAILER!!! (last of file)*/                                \
        }                                                                                  \
        something_to_run_in_loop                                                           \
    }

// Using newc archive format
struct cpio_newc_header
{
    char c_magic[6]; // fixed, "070701".
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
};

/* write pathname, data, next header into corresponding parameter*/
int cpio_newc_parse_header(struct cpio_newc_header *this_header_pointer,
                           char **pathname, unsigned int *filesize, char **data,
                           struct cpio_newc_header **next_header_pointer);

unsigned int padding_4byte(unsigned int size);
int cpio_get_file(char *filepath, unsigned int *c_filesize, char **c_filedata);

#endif /* _CPIO_H_ */