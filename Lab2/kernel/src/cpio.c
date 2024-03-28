#include "cpio.h"
#include "utils.h"

#define HEADER cpio_newc_header

/* write pathname, data, next header into corresponding parameter */
/* if no next header, next_header = 0 */
/* return -1 if parse error*/
int cpio_newc_parse_header(
    cpio_newc_header *curr_header,
    char **pathname,
    unsigned int *filesize,
    char **data,
    cpio_newc_header **next_header)
{
    /* Ensure magic header exists. */
    if (strncmp(curr_header->c_magic, CPIO_NEWC_MAGIC_NUM, sizeof(curr_header->c_magic)))
        return -1;

    // transfer big endian 8 byte hex string to unsigned int and store into *filesize
    *filesize = hex_to_decn(curr_header->c_filesize, 8);

    // end of header is the pathname
    *pathname = ((char *)curr_header) + sizeof(HEADER);

    // get file data, file data is just after pathname
    unsigned int pathname_length = hex_to_decn(curr_header->c_namesize, 8);
    unsigned int offset = pathname_length + sizeof(HEADER);
    // The file data is padded to a multiple of four bytes
    offset = offset % 4 == 0 ? offset : (offset + 4 - offset % 4);
    *data = (char *)curr_header + offset;

    // get next header pointer
    if (*filesize == 0)
    {
        *next_header = (HEADER *)*data;
    }
    else
    {
        offset = *filesize;
        *next_header = (HEADER *)(*data + (offset % 4 == 0 ? offset : (offset + 4 - offset % 4)));
    }

    // if filepath is TRAILER!!! means there is no more files.
    if (!strncmp(*pathname, "TRAILER!!!", sizeof("TRAILER!!!")))
        *next_header = 0;

    return 0;
}