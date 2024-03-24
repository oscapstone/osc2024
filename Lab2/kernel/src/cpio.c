#include "cpio.h"
#include "def.h"
#include "memory.h"
#include "mini_uart.h"
#include "string.h"

static inline void
cpio_list(char* name_addr, unsigned int name_size)
{
    for (unsigned int i = 0; i < name_size; i++)
        uart_send(name_addr[i]);
    uart_send_string("\n");
}

static inline void cpio_cat(char* file_addr, unsigned int file_size)
{
    for (unsigned int i = 0; i < file_size; i++)
        uart_send(file_addr[i]);
    uart_send('\r');
}

static int cpio_parse(enum cpio_mode mode, char* file_name)
{
    char* cpio = (char*)CPIO_ADDR;
    struct cpio_newc_header* header = (struct cpio_newc_header*)cpio;

    while (!strncmp(header->c_magic, CPIO_MAGIC, strlen(CPIO_MAGIC))) {

        char* file = (char*)header + sizeof(struct cpio_newc_header);

        if (!strcmp(file, CPIO_MAGIC_FOOTER))
            return mode == CPIO_CAT ? EXIT_FAILURE : EXIT_SUCCESS;

        unsigned int name_size = hexstr2int(header->c_namesize);
        unsigned int file_size = hexstr2int(header->c_filesize);
        char* file_content = (char*)mem_align(file + name_size, 4);

        switch (mode) {
        case CPIO_LIST:
            cpio_list(file, name_size);
            break;

        case CPIO_CAT:
            if (!strcmp(file, file_name)) {
                cpio_cat(file_content, file_size);
                return EXIT_SUCCESS;
            }
            break;

        default:
            break;
        }

        header =
            (struct cpio_newc_header*)mem_align(file_content + file_size, 4);
    }

    return EXIT_FAILURE;
}

void ls(void)
{
    int status = cpio_parse(CPIO_LIST, NULL);
    if (status == EXIT_FAILURE)
        uart_send_string("Parse Error\n");
}

void cat(char* file_name)
{
    int status = cpio_parse(CPIO_CAT, file_name);
    if (status == EXIT_FAILURE) {
        uart_send_string("File '");
        uart_send_string(file_name);
        uart_send_string("' not found\n");
    }
}
