#include "cpio.h"
#include "initramfs.h"
#include "malloc.h"
#include "string.h"
#include "type.h"
#include "uart.h"

static cpio_meta_t *head = NULL;
void parse_initramfs(int addr)
{
    cpio_hdr_t *hdr = (cpio_hdr_t *) ((long) addr);

    char *c;
    int padding, namesize, filesize;
    cpio_meta_t *cur = init_cpio_meta();
    head = cur;

    while (1) {
        namesize = hex_atoi(hdr->c_namesize, 8);
        filesize = hex_atoi(hdr->c_filesize, 8);
        c = (char *) (hdr+1);

        cur->filename = c;
        cur->namesize = namesize;
        padding = PADDING_4(sizeof(cpio_hdr_t) + namesize);
        c = (char *) hdr + padding;

        cur->content = c;
        cur->filesize = filesize;
        padding = PADDING_4(filesize);
        c += padding;

        if (!strcmp(cur->filename, "TRAILER!!!")) {
            cur->prev->next = NULL;
            break;
        }

        cur->next = init_cpio_meta();
        cur->next->prev = cur;
        cur = cur->next;
        hdr = (cpio_hdr_t *) c;
    }

    uart_puts("initramfs parse complete :D\n");
}

void list_initramfs()
{
    cpio_meta_t *cur = head;
    while (cur != NULL) {
        char *c = cur->filename;

        for (int i = 0; i < cur->namesize; i++) {
            uart_send(c[i]);
        }
        uart_puts("\n");

        cur = cur->next;
    }
}

cpio_meta_t *find_initramfs(const char *s)
{
    cpio_meta_t *cur = head;
    while (cur != NULL) {
        if (!strcmp(cur->filename, s)) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

void cat_initramfs()
{
    uart_puts("Filename: ");

    char *line = NULL;
    getline(&line, 0x20);

    cpio_meta_t *f = find_initramfs(line);
    if (f == NULL) {
        return;
    }
    char *ch = f->content;
    for (int i = 0; i < f->filesize; i++) {
        uart_send(ch[i]);
    }
    uart_puts("\n");
}

int initramfs_callback(int addr)
{
    parse_initramfs(addr);
    return 0;
}