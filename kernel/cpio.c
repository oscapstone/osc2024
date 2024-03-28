#include "cpio.h"
#include "malloc.h"
#include "string.h"
#include "uart.h"

cpio_obj_t *init_cpio_obj()
{
    cpio_obj_t *cpio = NULL;
    cpio = (cpio_obj_t *) malloc(sizeof(cpio_obj_t));

    cpio_hdr_t *hdr = NULL;
    hdr = (cpio_hdr_t *) malloc(sizeof(cpio_hdr_t));

    cpio_meta_t *meta = NULL;
    meta = (cpio_meta_t *) malloc(sizeof(cpio_meta_t));
    meta->filename = NULL;
    meta->content = NULL;

    cpio_ops_t *ops = NULL;
    ops = (cpio_ops_t *) malloc(sizeof(cpio_ops_t));

    cpio->hdr = hdr;
    cpio->meta = meta;
    cpio->ops = ops;
    cpio->next = NULL;

    return cpio;
}

void cpio_cat()
{
    uart_puts("cat file!\n");
}

void cpio_stat()
{
    uart_puts("stat file!\n");
}

/**
 * Parse CPIO Object using recursive descent parser
 * 
 * Return:
 *      0   :  Failed
 *      >0  :  Success, indicating how many bytes were eaten
*/
int parse(cpio_obj_t *cpio, char *initramfs)
{
    char *original = initramfs;
    int eaten = 0;

    if ((eaten = parse_c_magic(cpio, initramfs)) != 6) {
        uart_puts("failed to parse magic\n");
        return 0;
    }
    initramfs += eaten;

    // parse the rest 13 header fields
    for (int i = 0; i < 13; i++) {
        if ((eaten = parse_8_bytes(*(&cpio->hdr->c_ino + i), initramfs)) != 8) {
            uart_puts("failed to parse xxx\n");
            return 0;
        }
        initramfs += eaten;
    }

    // parse filename
    eaten = parse_filename(cpio, initramfs);
    initramfs += eaten;

    // parse content
    eaten = parse_content(cpio, initramfs);
    initramfs += eaten;

    return initramfs - original;
}

// Return how many bytes are eaten
int parse_c_magic(cpio_obj_t *cpio, char *pos)
{
    char magic[] = { '0', '7', '0', '7', '0', '1' };
    int i = 0;
    for (i = 0; i < 6; i++) {
        if (*(pos++) != magic[i]) {
            break;
        }
        cpio->hdr->c_magic[i] = magic[i];
    }
    return i;
}

int parse_8_bytes(char *c, char *pos)
{
    int i = 0;
    for (i = 0; i < 8; i++) {
        c[i] = *(pos++);
    }
    return i;
}

int parse_filename(cpio_obj_t *cpio, char *pos)
{
    int namesize = hex_atoi(cpio->hdr->c_namesize, 8);
    if (!namesize) {
        return 0;
    }

    int eaten = 0, i = 0;
    char *filename = (char *) malloc(sizeof(char) * namesize);
    for (i = 0; i < namesize; i++) {
        filename[i] = *pos;
        pos++;
        eaten++;
    }
    cpio->meta->filename = filename;

    return eaten;
}

int parse_content(cpio_obj_t *cpio, char *pos)
{
    int filesize = hex_atoi(cpio->hdr->c_filesize, 8);
    if (!filesize) {
        return 0;
    }

    int eaten = 0;
    char *content = (char *) malloc(sizeof(char) * filesize);
    pos++;
    for (int i = 0; i < filesize; i++) {
        content[i] = *pos;
        pos++;
        eaten++;
    }

    cpio->meta->content = content;

    return eaten;
}
