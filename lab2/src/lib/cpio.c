#include "cpio.h"
#include "string.h"
#include "uart.h"

static unsigned long align_up(unsigned long n, unsigned long align) { return (n + align - 1) & (~(align - 1)); }

int cpio_parse_header(struct cpio_newc_header *archive, char **filename, unsigned int *_filesize, void **data,
                      struct cpio_newc_header **next)
{
    // ensure magic header exists
    if (strncmp(archive->c_magic, "070701", sizeof(archive->c_magic)) != 0)
        return -1;

    unsigned int filesize = parse_hex_str(archive->c_filesize, sizeof(archive->c_filesize));
    *filename = ((char *)archive) + sizeof(struct cpio_newc_header);

    // ensure filename isn't the trailer indicating EOF
    if (strncmp(*filename, "TRAILER!!!", sizeof("TRAILER!!!") - 1) == 0) {
        *next = 0;
        return 1;
    }

    // find offset to data
    int filename_length = parse_hex_str(archive->c_namesize, sizeof(archive->c_namesize));
    *data = (void *)align_up(((unsigned long)archive) + sizeof(struct cpio_newc_header) + filename_length, 4);
    *next = (struct cpio_newc_header *)align_up((unsigned long)(*data) + filesize, 4);

    if (_filesize)
        *_filesize = filesize;

    return 0;
}

void cpio_ls()
{
    char *current_filename;
    void *filedata;
    unsigned int size;
    struct cpio_newc_header *header = (struct cpio_newc_header *)(0x20000000);

    while (header) {
        int error = cpio_parse_header(header, &current_filename, &size, &filedata, &header);
        if (error)
            break;

        uart_puts(current_filename);
        uart_puts("\n");
    }
}

void cpio_cat(char *file)
{
    char *current_filename;
    void *filedata;
    unsigned int size;
    struct cpio_newc_header *header = (struct cpio_newc_header *)(0x20000000);

    while (header) {
        int error = cpio_parse_header(header, &current_filename, &size, &filedata, &header);
        if (header == 0)
            uart_puts("No such file or directory\r\n");
        if (error)
            break;
        if (!strcmp(current_filename, file)) {
            for (unsigned int i = 0; i < size; i++)
                uart_send(((char *)filedata)[i]);
            uart_puts("\n");
            return;
        }
    }
}
