#include"header/cpio.h"
#include"header/utils.h"
char *cpio_start;
char *cpio_end;
int cpio_newc_parse_header(struct cpio_newc_header *this_header_pointer, char **pathname, unsigned int *filesize, char **data, struct cpio_newc_header **next_header_pointer)
{   
    // Ensure magic header 070701 
    // new ascii format
    if (strncmp(this_header_pointer->c_magic, CPIO_NEWC_HEADER_MAGIC, sizeof(this_header_pointer->c_magic)) != 0){
        return -1;
    }
    // transfer big endian 8 byte hex string to unsinged int 
    // data size
    *filesize = hex_to_int(this_header_pointer->c_filesize, 8);

    // end of header is the pathname
    // header | pathname str | data
    *pathname = ((char *)this_header_pointer) + sizeof(struct cpio_newc_header);
    // uart_send_str(pathname[0]);
    // get file data, file data is just after pathname
    // header | pathname str | data
    // check picture on hackmd note
    unsigned int pathname_length = hex_to_int(this_header_pointer->c_namesize, 8);
    // get the offset to start of data
    // | offset | data
    unsigned int offset = pathname_length + sizeof(struct cpio_newc_header);
    // pathname and data might be zero padding
    // section % 4 ==0
    offset = offset % 4 == 0 ? offset : (offset + 4 - offset % 4); // padding
    // header pointer + offset = start of data
    // h| offset | data
    *data = (char *)this_header_pointer + offset;

    // get next header pointer
    if (*filesize == 0)
        // hardlinked files handeld by setting filesize to zero
        *next_header_pointer = (struct cpio_newc_header *)*data;
    else
    {
        // data size
        offset = *filesize;
        // move pointer to the end of data
        *next_header_pointer = (struct cpio_newc_header *)(*data + (offset % 4 == 0 ? offset : (offset + 4 - offset % 4)));
    }

    // if filepath is TRAILER!!! means there is no more files.
    // end of archieve
    // empty filename : TRAILER!!!
    if (strncmp(*pathname, "TRAILER!!!", sizeof("TRAILER!!!")) == 0)
        *next_header_pointer = 0;

    return 0;
}
int ls(){
    // cpio_start = (char*)0x8000000 ;
    // cpio_start = (char*)0x20000000;
    char *filepath;
    char *filedata;
    unsigned int filesize;
    // current pointer
    struct cpio_newc_header *header_pointer = (struct cpio_newc_header *)cpio_start;

    // print every cpio pathname
    while (header_pointer)
    {
        int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
        // if parse header error
        if (error)
        {
            uart_send_str("error\n");
            break;
        }

        // if this is not TRAILER!!! (last of file)
        if (header_pointer != 0){
            uart_send_str(filepath);
            uart_send_str("\n");
        }
    }
    return 0;
}
int cat(char *path){
    // cpio_start = (char*) 0x8000000;
    // cpio_start = (char*)0x20000000;
    char *filepath;
    char *filedata;
    unsigned int filesize;
    // current header pointer, cpio start
    struct cpio_newc_header *header_pointer = (struct cpio_newc_header *)cpio_start;

    while (header_pointer)
    {   
        int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
        // if parse header error
        if (error == -1)
        {
            uart_send_str("error \n");
            break;
        }
        // parse until filepath is same as cat input
        // print the content of input file
        if (string_compare(path, filepath))
        {
            if(string_compare(filepath,".")){
                uart_send_str("cat: ");
                uart_send_str(path);
                uart_send_str("No such file or directory\r\n");
                break;
            }
            for (unsigned int i = 0; i < filesize; i++)
                uart_send_char(filedata[i]);
            uart_send_str("\n");
            break;
        }
        // end of cpio, cannot find input file
        if (header_pointer == 0){
            uart_send_str("cat: \"");
            uart_send_str(path);
            uart_send_str("\" No such file or directory\r\n");
            break;
        }
            
    }
    return 0;
}