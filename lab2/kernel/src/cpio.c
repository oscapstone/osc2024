#include "cpio.h"
#include "utils.h"

/* Parse an ASCII hex string into an integer. (big endian)*/
static unsigned int parse_hex_str(char *s, unsigned int max_len)
{
    unsigned int r = 0;

    for (unsigned int i = 0; i < max_len; i++) {
        r *= 16;
        if (s[i] >= '0' && s[i] <= '9') {
            r += s[i] - '0';
        }  else if (s[i] >= 'a' && s[i] <= 'f') {
            r += s[i] - 'a' + 10;
        }  else if (s[i] >= 'A' && s[i] <= 'F') {
            r += s[i] - 'A' + 10;
        } else {
            return r;
        }
    }
    return r;
}


/* write pathname, data, next header into corresponding parameter */
/* if no next header, next_header_pointer = 0 */
/* return -1 if parse error*/
int cpio_newc_parse_header(struct cpio_newc_header *this_header_pointer, char **pathname, unsigned int *filesize, char **data, struct cpio_newc_header **next_header_pointer)
{
    /* Ensure magic header exists. */
    if (strncmp(this_header_pointer->c_magic, CPIO_NEWC_HEADER_MAGIC, sizeof(this_header_pointer->c_magic)) != 0) return -1;

    // transfer big endian 8 byte hex string to unsigned int and store into *filesize
    *filesize = parse_hex_str(this_header_pointer->c_filesize,8);

    // end of header is the pathname
    *pathname = ((char *)this_header_pointer) + sizeof(struct cpio_newc_header); 

    // get file data, file data is just after pathname
    unsigned int pathname_length = parse_hex_str(this_header_pointer->c_namesize,8);
    unsigned int offset = pathname_length + sizeof(struct cpio_newc_header);
    // The file data is padded to a multiple of four bytes
    offset = offset % 4 == 0 ? offset:(offset+4-offset%4);
    *data = (char *)this_header_pointer+offset;

    //get next header pointer
    if(*filesize==0)
    {
        *next_header_pointer = (struct cpio_newc_header*)*data;
    }else
    {
        offset = *filesize;
        *next_header_pointer = (struct cpio_newc_header*)(*data + (offset%4==0?offset:(offset+4-offset%4)));
    }

    // if filepath is TRAILER!!! means there is no more files.
    if(strncmp(*pathname,"TRAILER!!!", sizeof("TRAILER!!!"))==0)
    {
        *next_header_pointer = 0;
    }

    return 0;
}
