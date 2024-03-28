#include "cpio.h"
#include "initramfs.h"
#include "malloc.h"
#include "string.h"
#include "uart.h"

static cpio_obj_t *head = NULL;
void parse_initramfs()
{
    char *initramfs = (char *) 0x8000000;   // qemu addr

    int eaten;
    cpio_obj_t *cpio = init_cpio_obj();
    cpio_obj_t *prev = NULL;    // for remove TRAILER
    head = cpio;

    // New
    while ((eaten = parse(cpio, initramfs)) != 0) {
        if (!strcmp(cpio->meta->filename, "TRAILER!!!")) {
            prev->next = NULL;
            break;
        }
        initramfs += eaten;
        while (*initramfs != '0') { // prevent content alignment
            initramfs++;
        }
        cpio->next = init_cpio_obj();
        prev = cpio;
        cpio = cpio->next;
    }

    uart_puts("initramfs parse complete^^\n");
}

void list_initramfs()
{
    cpio_obj_t *cur = head;
    while (cur != NULL) {
        uart_puts(cur->meta->filename);
        uart_puts("\n");

        cur = cur->next;
    }
}

void cat_initramfs()
{
    uart_puts("Filename: ");

    char line[1024];
    char c;
    int idx = 0;
    while ((c = (char) uart_getc()) != '\n') {
        line[idx++] = c;
        uart_send(c);
    }
    line[idx++] = '\0';
    uart_send('\n');

    cpio_obj_t *cur = head;
    while (cur != NULL) {
        if (!strcmp(cur->meta->filename, line)) {
            uart_puts(cur->meta->content);
            break;
        }
        cur = cur->next;
    }
}
