#include "cpio.h"
#include "utils.h"
#include "mini_uart.h"

char *CPIO_START;
char *CPIO_END;

static unsigned int ascii_hex_to_int(char *s, unsigned int max_len) {
    unsigned int hex_base = 0;
    for (unsigned int i=0; i<max_len; i++) {
        hex_base *= 16;
        if (s[i] >= '0' && s[i] <= '9') 
            hex_base += s[i] - '0';
        else if (s[i] >= 'a' && s[i] <= 'f') 
            hex_base += s[i] - 'a' + 10;
        else if (s[i] >= 'A' && s[i] <= 'F')
            hex_base += s[i] - 'A' + 10;
        else
            return hex_base;
    }
    return hex_base;
}

int cpio_parse_header(struct cpio_newc_header *header_ptr, char **pathname, 
    unsigned int *filesize, char **data, struct cpio_newc_header **next_header_ptr) {
    
    if (strncmp(header_ptr->c_magic, CPIO_NEWC_HEADER_MAGIC, sizeof(header_ptr->c_magic)) != 0) return -1;

    // uart_puts(header_ptr->c_filesize);
    // uart_puts("\r\n");

    *filesize = ascii_hex_to_int(header_ptr->c_filesize, 8);
    *pathname = ((char *) header_ptr) + sizeof(struct cpio_newc_header);

    unsigned int pathname_len = ascii_hex_to_int(header_ptr->c_namesize, 8);
    unsigned int offset = pathname_len + sizeof(struct cpio_newc_header);
    
    /* If offset is multiple of 4 -> offset + 4 - 0 
       if not -> offset + (4 - r) 就會增加到下一個四的倍數 */
    offset = offset % 4 == 0 ? offset:(offset+4-offset%4);
    *data = (char *)header_ptr + offset;

    // Get next header pointer
    if(*filesize==0) {
        *next_header_ptr = (struct cpio_newc_header*)*data;
    } else {
        offset = *filesize;
        *next_header_ptr = (struct cpio_newc_header*)(*data + (offset%4==0?offset:(offset+4-offset%4)));
    }

    // if filepath is TRAILER!!! means there is no more files.
    if(strncmp(*pathname, "TRAILER!!!", sizeof("TRAILER!!!")) == 0) {
        *next_header_ptr = 0;
    }

    return 0;
}   

/* write pathname, data, next header into corresponding parameter */
/* if no next header, next_header_pointer = 0, return 1 */
/* return -1 if parse error*/
CPIO_return_t cpio_newc_parse_header(struct cpio_newc_header *this_header_pointer, char **pathname, unsigned int *filesize, char **data, struct cpio_newc_header **next_header_pointer)
{
    /* Ensure magic header exists. */
    if (strncmp(this_header_pointer->c_magic, CPIO_NEWC_HEADER_MAGIC, sizeof(this_header_pointer->c_magic)) != 0)
        return CPIO_ERROR;

    // transfer big endian 8 byte hex string to unsigned int and store into *filesize
    *filesize = ascii_hex_to_int(this_header_pointer->c_filesize, 8);

    // end of header is the pathname
    *pathname = ((char *)this_header_pointer) + sizeof(struct cpio_newc_header);

    // get file data, file data is just after pathname
    unsigned int pathname_length = ascii_hex_to_int(this_header_pointer->c_namesize, 8);
    unsigned int offset = pathname_length + sizeof(struct cpio_newc_header);
    // The file data is padded to a multiple of four bytes
    offset = padding_4byte(offset);
    *data = (char *)this_header_pointer + offset;

    // get next header pointer
    if (*filesize == 0) {
        *next_header_pointer = (struct cpio_newc_header *)*data;
    } else {
        offset = *filesize;
        *next_header_pointer = (struct cpio_newc_header *)(*data + padding_4byte(offset));
    }

    // if filepath is TRAILER!!! means there is no more files.
    if (strncmp(*pathname, "TRAILER!!!", sizeof("TRAILER!!!")) == 0) {
        *next_header_pointer = 0;
        return CPIO_TRAILER;
    }

    return CPIO_SUCCESS;
}

unsigned int padding_4byte(unsigned int size) {
    return size + (4 - size % 4) % 4;
}

/* Get filesize and filedata by filepath */
int cpio_get_file(const char *filepath, unsigned int *c_filesize, char **c_filedata) {
    char *c_filepath;
    int error;

    CPIO_FOR_EACH(&c_filepath, c_filesize, c_filedata, error, {
        if (strcmp(c_filepath, filepath) == 0)
        {
            return CPIO_SUCCESS;
        }
    });
    return error;
}