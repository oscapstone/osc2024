#ifndef _CPIO_H
#define _CPIO_H

// cpio officail format : https://manpages.ubuntu.com/manpages/bionic/en/man5/cpio.5.html

#define CPIO_NEWC_HEADER_MAGIC "070701" //use to check bigE or littleE
#define CPIO_PLACE 0x8000000
#define CPIO_FOR_EACH(c_filepath, c_filesize, c_filedata, error, something_to_run_in_loop) \
    do                                                                                     \
    {                                                                                      \
        struct cpio_newc_header *pos = (struct cpio_newc_header *)CPIO_PLACE;              \
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

struct cpio_newc_header{
    char c_magic[6];            // fixed, "070701".
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
typedef enum
{
    CPIO_TRAILER = 0,
    CPIO_SUCCESS = 1,
    CPIO_ERROR = -1
} CPIO_return_t;
void cpio_ls(void * archive);
void cpio_cat(void * archive , char *filename);

int cpio_newc_parse_header(struct cpio_newc_header *this_header_pointer, char **pathname, unsigned int *filesize,
                                char ** data, struct cpio_newc_header **next_header_pointer);

int cpio_get_file(const char *filepath, unsigned int *c_filesize, char **c_filedata);




#endif