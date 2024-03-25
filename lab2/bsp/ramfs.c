#include "kernel/bsp/ramfs.h"

#include "kernel/bsp/uart.h"
#include "lib/stdlib.h"
#include "lib/string.h"
#include "lib/utils.h"

// #define CPIO_BASE_QEMU (0x8000000)
#define CPIO_BASE_RPI (0x20000000)
static void *ramfs_base = (void *)CPIO_BASE_RPI;  // DEFAULT for RPI

static int hextoi(char *s, int n);

void init_ramfs_callback(void *addr) {
    uart_puts("[INFO] Initrd is mounted at ");
    uart_hex((uint32_t)addr);
    uart_send('\n');
    ramfs_base = (uint32_t *)addr;
}

static FileList file_list;

FileList *ramfs_get_file_list() {
    uint8_t *fptr = (uint8_t *)ramfs_base;  // for 8 byte alignment
    file_list.file_count = 0;               // 重置檔案數量

    while (strcmp((void *)fptr + sizeof(cpio_t), "TRAILER!!!") &&
           file_list.file_count < MAX_FILES) {
        char *file_name = (void *)fptr + sizeof(cpio_t);

        cpio_t *header = (cpio_t *)fptr;
        uint32_t namesize = hextoi(header->c_namesize, 8);
        uint32_t filesize = hextoi(header->c_filesize, 8);
        uint32_t copy_length =
            namesize < MAX_FILENAME_LENGTH ? namesize : MAX_FILENAME_LENGTH - 1;

        strncpy(file_list.file_names[file_list.file_count], file_name,
                copy_length);
        file_list.file_names[file_list.file_count][copy_length] = '\0';
        file_list.file_count++;

        uint32_t headsize = align4(sizeof(cpio_t) + namesize);
        uint32_t datasize = align4(filesize);

        fptr += headsize + datasize;
    }

    return &file_list;
}

static char file_buf[1024];
char *ramfs_get_file_contents(char *file_name) {
    uint8_t *fptr = (uint8_t *)ramfs_base;  // for 8 byte alignment
    while (strcmp((void *)fptr + sizeof(cpio_t), "TRAILER!!!")) {
        cpio_t *header = (cpio_t *)fptr;
        uint32_t namesize = hextoi(header->c_namesize, 8);
        uint32_t filesize = hextoi(header->c_filesize, 8);

        uint32_t headsize = align4(sizeof(cpio_t) + namesize);
        uint32_t datasize = align4(filesize);

        if (strcmp((void *)fptr + sizeof(cpio_t), file_name) == 0) {
            strncpy(file_buf, (void *)fptr + headsize, filesize);
            file_buf[filesize] = '\0';
            return file_buf;
        } else {
        }

        fptr += headsize + datasize;
    }
    return NULL;
}

static int hextoi(char *s, int n) {
    int r = 0;
    while (n-- > 0) {
        r = r << 4;
        if (*s >= 'A')
            r += *s++ - 'A' + 10;
        else if (*s >= 0)
            r += *s++ - '0';
    }
    return r;
}
