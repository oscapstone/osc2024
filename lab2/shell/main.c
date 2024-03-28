#include "headers/shell.h"
#include "headers/uart.h"
#include "headers/dtb.h"

extern void *_dtb_ptr;

void main()
{
    init();
    fdt_traverse(initramfs_callback, _dtb_ptr);
    display("Here is the actual kernel.\n");
    shell();
    return;
}