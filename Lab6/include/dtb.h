#include <stdint.h>
#include <stddef.h>
#ifndef _DTB_H_
#define _DTB_H_

#define uint32_t unsigned int
#define uint64_t unsigned long long

// manipulate device tree with dtb file format
// linux kernel fdt.h
#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE 0x00000002
#define FDT_PROP 0x00000003
#define FDT_NOP 0x00000004
#define FDT_END 0x00000009

extern char* dtb_ptr;

typedef void (*dtb_callback)(uint32_t node_type, char *name, void *value, uint32_t name_size);

uint32_t uint32_endian_big2lttle(uint32_t data);
uint64_t uint64_endian_big2lttle(uint64_t data);
uint32_t utils_align_up(uint32_t size, int alignment);
size_t utils_strlen(const char *s);

void traverse_device_tree(void *base, dtb_callback callback);  //traverse dtb tree
void dtb_cb_print(uint32_t type, char *name, void *data, uint32_t name_size);
void dtb_cb_init(uint32_t type, char *name, void *data, uint32_t name_size);

void dtb_reserved_memory();

#endif