#include "cpio.h"
#include "def.h"
#include "memory.h"
#include "mini_uart.h"
#include "string.h"

/* CPIO archive format
 * https://man.freebsd.org/cgi/man.cgi?query=cpio&sektion=5
 */

/* CPIO new ASCII format
 * The  "new"  ASCII format uses 8-byte hexadecimal fields for all numbers
 */
struct __attribute__((packed)) cpio_newc_header {
    char c_magic[6];  // "070701"
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

enum cpio_mode { CPIO_LIST, CPIO_CAT };

static inline void cpio_print_filename(char* name_addr, unsigned int name_size)
{
    for (unsigned int i = 0; i < name_size; i++)
        uart_send(name_addr[i]);
    uart_send_string("\n");
}

static inline void cpio_print_content(char* file_addr, unsigned int file_size)
{
    for (unsigned int i = 0; i < file_size; i++) {
        if (file_addr[i] == '\n')
            uart_send('\r');
        uart_send(file_addr[i]);
    }
}


/*
 * CPIO archive will be stored like this:
 *
 * +--------------------------+
 * |  struct cpio_newc_header |
 * +--------------------------+
 * |       (file name)        |
 * +--------------------------+
 * |  0 padding(4-byte align) |
 * +--------------------------+
 * |       (file data)        |
 * +--------------------------+
 * |  0 padding(4-byte align) |
 * +--------------------------+
 * |  struct cpio_newc_header |
 * +--------------------------+
 * |            .             |
 * |            .             |
 * |            .             |
 * +--------------------------+
 * |  struct cpio_newc_header |
 * +--------------------------+
 * |        TRAILER!!!        |
 * +--------------------------+
 */

static uintptr_t _cpio_ptr;

uintptr_t get_cpio_ptr(void)
{
    return _cpio_ptr;
}

void set_cpio_ptr(uintptr_t ptr)
{
    _cpio_ptr = ptr;
}

static int cpio_parse(enum cpio_mode mode, char* file_name)
{
    char* cpio = (char*)_cpio_ptr;
    struct cpio_newc_header* header = (struct cpio_newc_header*)cpio;

    while (!str_n_cmp(header->c_magic, CPIO_MAGIC, str_len(CPIO_MAGIC))) {
        char* file = (char*)header + sizeof(struct cpio_newc_header);

        if (!str_cmp(file, CPIO_MAGIC_FOOTER))
            return mode == CPIO_CAT ? CPIO_EXIT_ERROR : CPIO_EXIT_SUCCESS;

        unsigned int name_size = hexstr2int(header->c_namesize);
        unsigned int file_size = hexstr2int(header->c_filesize);
        char* file_content = (char*)mem_align(file + name_size, 4);

        switch (mode) {
        case CPIO_LIST:
            cpio_print_filename(file, name_size);
            break;

        case CPIO_CAT:
            if (!str_cmp(file, file_name)) {
                cpio_print_content(file_content, file_size);
                return CPIO_EXIT_SUCCESS;
            }
            break;

        default:
            break;
        }

        header =
            (struct cpio_newc_header*)mem_align(file_content + file_size, 4);
    }

    return CPIO_EXIT_ERROR;
}

void ls(void)
{
    int status = cpio_parse(CPIO_LIST, NULL);
    if (status == CPIO_EXIT_ERROR)
        uart_send_string("Parse Error\n");
}

void cat(char* file_name)
{
    int status = cpio_parse(CPIO_CAT, file_name);
    if (status == CPIO_EXIT_ERROR) {
        uart_send_string("File '");
        uart_send_string(file_name);
        uart_send_string("' not found\n");
    }
}
