#include "cpio.h"
#include "shell.h"

// due to the reason of the data stored in Hex, we want to show the strings that we can recongize
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
static int cpio_strncmp(const char *a, const char *b, unsigned long n)
{
    unsigned long i;
    for (i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            return a[i] - b[i];
        }
        if (a[i] == 0) {
            return 0;
        }
    }
    return 0;
}
// 
int cpio_newc_parse_header(struct cpio_newc_header *this_header_pointer, char **pathname, unsigned int *filesize,
                                char ** data, struct cpio_newc_header **next_header_pointer)
{
    if (cpio_strncmp(this_header_pointer->c_magic, CPIO_NEWC_HEADER_MAGIC, sizeof(this_header_pointer->c_magic))!= 0) return -1;

    //c_filesize is in the struct cpio_newc_header, and with 8bytes
    *filesize = parse_hex_str(this_header_pointer->c_filesize,8);

    // pathname is followed by the end of the cpio format structure
    *pathname = ((char *) this_header_pointer) + sizeof(struct cpio_newc_header);

    //in order to get the file data which is located at the position after pathname
    // c_namesize present the bytes of pathname
    unsigned int pathname_length = parse_hex_str(this_header_pointer->c_namesize,8);
    // 
    unsigned int offset = pathname_length + sizeof(struct cpio_newc_header);
    //since the data will be align with multiple of four.
    offset = offset % 4 == 0 ? offset : (offset + 4 - offset %4);

    *data = (char * ) this_header_pointer + offset;

    // for the next header ptr
    if (*filesize == 0){
        *next_header_pointer = (struct cpio_newc_header *)*data;
    } else{
        offset = *filesize;
        *next_header_pointer = (struct cpio_newc_header*)(*data + (offset%4==0?offset:(offset+4-offset%4)));
    }

    // if the filepath is "TRAILER!!!" which means there is no more file.
    if (cpio_strncmp(*pathname, "TRAILER!!!", sizeof("TRAILER!!!")) == 0){
        *next_header_pointer = 0;
    }
    
    return 0;
};

