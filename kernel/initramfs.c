#include "initramfs.h"
#include "uart.h"

// TODO: 定義一些 function pointer 在 struct 裏面
void parse_initramfs()
{
    cpio_object_t objs[100];
    char *initramfs = (char *) 0x8000000;   // qemu addr
    while (1) {
        char c = *initramfs;
        uart_send(c);
        // create obj, only save hdr and offset from start_addr
    }
}

void list_initramfs()
{
    // read objects
}

void cat_initramfs()
{
    // locate objects, and read its content
}