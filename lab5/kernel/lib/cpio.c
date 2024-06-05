#include "cpio.h"
#include "uart.h"
#include "string.h"
#include "utils.h"
#include "sched.h"

char *cpio_start;
char *cpio_end;

// ubuntu man page cpio
/* Parse one file system object */
/* write pathname,data,next header into corresponding parameter */
/* if no next header, next_header_pointer = 0 */
/* return -1 if parse error*/
int cpio_newc_parse_header(struct cpio_newc_header *this_header_pointer, char **pathname, unsigned int *filesize, char **data, struct cpio_newc_header **next_header_pointer)
{
    // Ensure magic header 070701 
    // new ascii format
    if (strncmp(this_header_pointer->c_magic, CPIO_NEWC_HEADER_MAGIC, sizeof(this_header_pointer->c_magic)) != 0)
        return -1;

    // transfer big endian 8 byte hex string to unsinged int 
    // data size
    *filesize = parse_hex_str(this_header_pointer->c_filesize, 8);

    // end of header is the pathname
    // header | pathname str | data
    *pathname = ((char *)this_header_pointer) + sizeof(struct cpio_newc_header);

    // get file data, file data is just after pathname
    // header | pathname str | data
    // check picture on hackmd note
    unsigned int pathname_length = parse_hex_str(this_header_pointer->c_namesize, 8);
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

int execfile(char *thefilepath)
{
    char* filepath;
    char* filedata;
    unsigned int filesize;
    struct cpio_newc_header *header_pointer = (struct cpio_newc_header *)cpio_start;

    while(header_pointer!=0)
    {
        int error = cpio_newc_parse_header(header_pointer,&filepath,&filesize,&filedata,&header_pointer);
        //if parse header error
        if(error)
        {
            uart_printf("error");
            break;
        }

        // traverse until find the filepath
        // program start address is data
        // header | pathname str | data
        if(!strcmp(thefilepath,filepath))
        {
            // 
            exec_thread(filedata, filesize);
            break;
        }

        //if this is TRAILER!!! (last of file)
        if (header_pointer == 0)
        {
            uart_printf("execfile: %s: No such file or directory\n", thefilepath);
            return -1;
        }
    }
    return 0;
}

int cat(char *thefilepath)
{
    char *filepath;
    char *filedata;
    unsigned int filesize;
    // current header pointer, cpio start
    struct cpio_newc_header *header_pointer = (struct cpio_newc_header *)cpio_start;

    while (header_pointer)
    {
        int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
        // if parse header error
        if (error)
        {
            uart_printf("error\n");
            break;
        }
        // parse until filepath is same as cat input
        // print the content of input file
        if (!strcmp(thefilepath, filepath))
        {
            for (unsigned int i = 0; i < filesize; i++)
                uart_printf("%c", filedata[i]);
            uart_printf("\n");
            break;
        }
        // end of cpio, cannot find input file
        if (header_pointer == 0)
            uart_printf("cat: %s: No such file or directory\r\n", thefilepath);
    }
    return 0;
}

int ls(char *working_dir)
{
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
            uart_printf("error\n");
            break;
        }

        // if this is not TRAILER!!! (last of file)
        if (header_pointer != 0)
            uart_printf("%s\n", filepath);
    }
    return 0;
}

char* get_file_start(char *thefilepath) {
    char *filepath;
    char *filedata;
    unsigned int filesize;
    struct cpio_newc_header *header_pointer = (struct cpio_newc_header *)cpio_start;

    while (header_pointer != 0) {
        int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
        //if parse header error
        if (error)
        {
            uart_puts("error");
            break;
        }

        if (strcmp(thefilepath, filepath) == 0)
        {
            return filedata;
        }

        //if this is TRAILER!!! (last of file)
        if (header_pointer == 0)
            uart_printf("execfile: %s: No such file or directory\r\n", thefilepath);
    }
    return 0;
}

unsigned int get_file_size(char *thefilepath) {
    char *filepath;
    char *filedata;
    unsigned int filesize;
    struct cpio_newc_header *header_pointer = (struct cpio_newc_header *)cpio_start;

    while (header_pointer != 0) {
        int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
        //if parse header error
        if (error)
        {
            uart_puts("error");
            break;
        }

        if (strcmp(thefilepath, filepath) == 0)
        {
            return filesize;
        }

        //if this is TRAILER!!! (last of file)
        if (header_pointer == 0)
            uart_printf("execfile: %s: No such file or directory\r\n", thefilepath);
    }
    return 0;
}