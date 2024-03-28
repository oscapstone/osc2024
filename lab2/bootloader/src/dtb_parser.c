#include "dtb_parser.h"
#include "uart.h"
#include "utils.h"

char *boot_dtb_ptr;

void fdt_check()
{
    uart_puts("checking fdt...\n");
    struct fdt_header *header = (struct fdt_header *)boot_dtb_ptr;
    
    if (fdt_u32_le2be(&(header->magic)) != 0xd00dfeed)
    {
        uart_puts("fdt error\n");
        return;
    }
    uart_puts("fdt pass\n");
}