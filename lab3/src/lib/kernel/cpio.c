#include "cpio.h"
#include "malloc.h"
#include "string.h"
#include "uart.h"

#define CPIO_NEWC_HEADER_MAGIC "070701"
#define CPIO_NEWC_TRAILER "TRAILER!!!"

struct cpio_newc_header { // 110 bytes
    char c_magic[6];
    char c_ino[8];  // inode number
    char c_mode[8]; // permissions
    char c_uid[8];
    char c_gid[8];
    char c_nlink[8]; // number of hard links
    char c_mtime[8]; // modification time
    char c_filesize[8];
    char c_devmajor[8];
    char c_devminor[8];
    char c_rdevmajor[8];
    char c_rdevminor[8];
    char c_namesize[8]; // size of filename in bytes
    char c_check[8];    // checksum
};

char *cpio_start;

static unsigned long align_up(unsigned long n, unsigned long align) { return (n + align - 1) & (~(align - 1)); }

int cpio_parse_header(struct cpio_newc_header *archive, char **filename, unsigned int *_filesize, void **data,
                      struct cpio_newc_header **next)
{
    // ensure magic header exists
    if (strncmp(archive->c_magic, CPIO_NEWC_HEADER_MAGIC, sizeof(archive->c_magic)) != 0)
        return -1;

    unsigned int filesize = parse_hex_str(archive->c_filesize, sizeof(archive->c_filesize));
    *filename = ((char *)archive) + sizeof(struct cpio_newc_header);

    // ensure filename isn't the trailer indicating EOF
    if (strncmp(*filename, CPIO_NEWC_TRAILER, sizeof(CPIO_NEWC_TRAILER)) == 0) {
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
    struct cpio_newc_header *header = (struct cpio_newc_header *)(cpio_start);

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
    struct cpio_newc_header *header = (struct cpio_newc_header *)(cpio_start);

    while (header) {
        int error = cpio_parse_header(header, &current_filename, &size, &filedata, &header);
        if (error) {
            uart_puts("No such file or directory\n");
            break;
        }
        if (!strcmp(current_filename, file)) {
            for (unsigned int i = 0; i < size; i++)
                uart_send(((char *)filedata)[i]);
            uart_puts("\n");
            return;
        }
    }
}

void cpio_exec(char *file)
{
    char *current_filename;
    void *filedata;
    unsigned int size;
    struct cpio_newc_header *header = (struct cpio_newc_header *)(cpio_start);

    while (header) {
        int error = cpio_parse_header(header, &current_filename, &size, &filedata, &header);
        if (error) {
            uart_puts("No such file or directory\n");
            break;
        }
        if (!strcmp(current_filename, file)) {
            char *user_sp = (char *)simple_malloc(4096);
            unsigned long spsr_el1 = 0x3c0;

            asm volatile("msr spsr_el1, %0\n\t"
                         "msr elr_el1, %1\n\t"
                         "msr sp_el0, %2\n\t"
                         "eret\n\t" ::"r"(spsr_el1),
                         "r"(filedata), "r"(user_sp + 4096));

            return;
        }
    }
}