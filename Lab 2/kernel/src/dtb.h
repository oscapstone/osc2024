#ifndef DTB_H
#define DTB_H

#include "types.h"


#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004
#define FDT_END         0x00000009


#define FDT_TRAVERSE_CORRECT        0
#define FDT_TRAVERSE_HEADER_ERROR   1
#define FDT_TRAVERSE_FORMAT_ERROR   2


typedef void (*fdt_callback)(uint32_t token, const byteptr_t name_ptr, const byteptr_t data_ptr, uint32_t v);

byteptr_t dtb_get_ptr();
void dtb_set_ptr(byteptr_t ptr);
uint32_t fdt_traverse(fdt_callback cb);

void fdt_find_initrd_addr(uint32_t token, const byteptr_t name_ptr, const byteptr_t data_ptr, uint32_t v);
void fdt_print_node(uint32_t token, const byteptr_t name_ptr, const byteptr_t, uint32_t);

#endif